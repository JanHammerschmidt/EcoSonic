#include "HID.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sysexits.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_time.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

//------------------------------------------------------------------------------------
// Globals

static const int NULLHandle = 0;
static uint64_t BaseTime;

//------------------------------------------------------------------------------------
// Exceptions & error handling


class IOException : public HIDException
{
public:
    IOException(const char* reason, IOReturn errorCode) 
    : HIDException(reason), myErrorCode(errorCode)
        {
            const char* ioReason = mach_error_string(errorCode);
            if (ioReason!=NULL)
                myIOReason = ioReason;
            else
                myIOReason = "<Unknown reason>";
        }
    virtual ~IOException() {}
    virtual const String GetReason() const { return myReason+": "+myIOReason; }
private:
    IOReturn    myErrorCode;
    String      myIOReason;
};


void ThrowOnError(int result, const char* msg)
{
    if (result!=0)
    {
        throw HIDException(msg);
    }
}


void ThrowOnIOError(IOReturn result, const char* msg)
{
	if (result != kIOReturnSuccess) 
	{
	    throw IOException(msg, result);
	}
}


//------------------------------------------------------------------------------------
// Type conversion & generally migrating data in & out of CoreFoundation structures

String
CFToStdLibraryString(CFStringRef str, int encoding = kCFStringEncodingUTF8)
{
    if (!str) return "<NULL>";
    CFIndex strLength = CFStringGetLength(str);
    unsigned int bufferSize = CFStringGetMaximumSizeForEncoding(strLength, encoding);
    char* buffer = new char[bufferSize];
    CFStringGetCString(str, buffer, bufferSize, encoding);
    String s(buffer);
    delete [] buffer;
    return s;
}

long
GetNumberFromDict(CFDictionaryRef dict, CFStringRef key)
{
    CFTypeRef object = CFDictionaryGetValue(dict, key);
    if (object == 0 || (CFGetTypeID(object) != CFNumberGetTypeID()))
        throw HIDException("Failed to get from dict");
    long number;
    if (!CFNumberGetValue((CFNumberRef) object, kCFNumberLongType, &number))
        throw HIDException("Failed to convert");
    return number;
}

uint64_t MachTimeToNanos(uint64_t mach_time)
{
    static mach_timebase_info_data_t timebase = {0,0};
    if (timebase.denom==0)
        (void) mach_timebase_info(&timebase);
    return mach_time * timebase.numer / timebase.denom;
}


//------------------------------------------------------------------------------------
// HID Elements -- the individual buttons, axes, and other widgets on your device:

class HIDElement : public IHIDElement
{
public:
                                HIDElement(unsigned int index, IOHIDDeviceInterface122** interface, CFDictionaryRef element);
    virtual                    ~HIDElement();
    virtual unsigned int        GetIndex()          const   { return myIndex;     }
    virtual unsigned int        GetUsage()          const   { return myUsage;     }
    virtual unsigned int        GetUsagePage()      const   { return myUsagePage; }
    virtual int                 GetValue()          const;
    virtual void                SetValue(int value);
    virtual int                 GetMinValue()       const   { return myMinValue;  }
    virtual int                 GetMaxValue()       const   { return myMaxValue;  }
    virtual float               GetNormalisedValue()const;
    virtual void                SetNormalisedRange(float min, float max);
    
                            
private:
    int                         myIndex;
    IOHIDElementCookie          myCookie;
    unsigned int                myUsage;
    unsigned int                myUsagePage;
    int                         myMinValue;
    int                         myMaxValue;
    IOHIDDeviceInterface122**   myInterface;
    float                       myOutputMin;
    float                       myOutputDelta;
    IOHIDElementType            myType;
    
    friend class                HIDevice;
};


HIDElement::HIDElement(unsigned int index, IOHIDDeviceInterface122** interface, CFDictionaryRef element)
: myIndex(index), myInterface(interface), myOutputMin(0.f), myOutputDelta(1.f)
{
    // Note that we don't acquire/release ownership of myInterface as HIDElements always
    // live as members of HIDevices (which do acquire/release ownership)
    myCookie = (IOHIDElementCookie) GetNumberFromDict(element, CFSTR(kIOHIDElementCookieKey));
    myUsage = GetNumberFromDict(element, CFSTR(kIOHIDElementUsageKey));
    myUsagePage = GetNumberFromDict(element, CFSTR(kIOHIDElementUsagePageKey));
    myType = (IOHIDElementType)GetNumberFromDict(element, CFSTR(kIOHIDElementTypeKey));
    try
    {
        myMinValue = GetNumberFromDict(element, CFSTR(kIOHIDElementMinKey));
        myMaxValue = GetNumberFromDict(element, CFSTR(kIOHIDElementMaxKey));
    }
    catch (HIDException&)
    {
        myMinValue = myMaxValue = 0;
    }
}

HIDElement::~HIDElement()
{
}

int
HIDElement::GetValue() const
{
    IOHIDEventStruct event;
    IOReturn result = (*myInterface)->getElementValue(myInterface, myCookie, &event);
    ThrowOnIOError(result,"Failed to get element value");
    return event.value;
}

void
HIDElement::SetValue(int value)
{
    //uint64_t timestamp = mach_absolute_time();
    IOHIDEventStruct event;
    IOReturn result = (*myInterface)->getElementValue(myInterface, myCookie, &event);
    event.value = value;
    // the last 4 parameters are simply documented 'UNSUPPORTED' with no further explanation :(
    // hopefully NULL is a sensible thing to pass.
    result = (*myInterface)->setElementValue(myInterface, myCookie, &event, 0, NULL, NULL, NULL);
    ThrowOnIOError(result,"Failed to set element value");
}

float
HIDElement::GetNormalisedValue() const
{
    float value = GetValue();
    // normalise to range 0-1
    value = (value-myMinValue)/(myMaxValue-myMinValue);
    // return in format requested by client app
    return myOutputMin + (myOutputDelta * value);
}

void
HIDElement::SetNormalisedRange(float min, float max) 
{ 
    myOutputMin   = min; 
    myOutputDelta = max-min;
}


//------------------------------------------------------------------------------------
// Human Interface Devices
// Initialise/Teardown

class HIDevice : public IHIDevice
{
public:
                                HIDevice(io_object_t hidDevice);
    virtual                    ~HIDevice();
    virtual const String        GetDeviceName() const { return myDeviceName; }
    virtual const String        GetManufacturerName() const { return myManufacturerName; }
    virtual const String        GetDeviceClassName() const { return myClassName; }
    virtual unsigned int        GetElementCount() const { return myElementCount; }
    virtual IHIDElement*        GetElement(unsigned int index);
    virtual const IHIDElement*  GetElement(unsigned int index) const;
    virtual void                StartQueue(unsigned int depth);
    virtual void                StartMonitoringElement(unsigned int index);
    virtual void                StopMonitoringElement(unsigned int index);
    virtual void                StopQueue();
    virtual bool                PollQueue(IHIDElement*& element, int& value, TimeUnits& time);

private:
    IOHIDDeviceInterface122**   myInterface;
    String                      myClassName;
    String                      myDeviceName;
    String                      myManufacturerName;
    HIDElement**                myElements;
    unsigned int                myElementCount;
    IOHIDQueueInterface **      myEventQueue;
    
    static CFMutableDictionaryRef CreateProperties(io_object_t hidDevice); // release them when you're done
    
    /* One day, this may become the start of device add/remove notification.
    
    io_object_t                 myNotification;
    
    static void interestCallback(void* refcon, io_service_t service, natural_t messageType, void* msgArg)
    {
        HIDevice* that = (HIDevice*) refcon;
        printf("%08x (%s) got %d (%x)\n", that, that->GetDeviceName().c_str(), messageType, messageType);
        if (messageType==kIOMessageServiceIsTerminated)
            that->NotifyDisconnected();
    }*/
};

HIDevice::HIDevice(io_object_t hidDevice) 
: IHIDevice(), myInterface(NULL), myElements(NULL), myElementCount(0), myEventQueue(NULL)
{
    IOReturn                ioReturnValue;

    io_name_t               className;
    ioReturnValue = IOObjectGetClass(hidDevice, className);
    ThrowOnIOError(ioReturnValue, "Failed to get class name.");

    myClassName = className;
    
    IOCFPlugInInterface**   plugInInterface = NULL;
    SInt32                  score = 0;
    ioReturnValue = IOCreatePlugInInterfaceForService(hidDevice,
                            kIOHIDDeviceUserClientTypeID,
                            kIOCFPlugInInterfaceID,
                            &plugInInterface,
                            &score);
    ThrowOnIOError(ioReturnValue, "Failed to create PlugInInterface.");

    //Call a method of the intermediate plug-in to create the device 
    //interface
    HRESULT                 plugInResult = S_OK;
    plugInResult = (*plugInInterface)->QueryInterface(plugInInterface,
                        CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID),
                        (void**) &myInterface);
    ThrowOnError(plugInResult != S_OK, "Couldn't create HID class device interface");
    (*plugInInterface)->Release(plugInInterface);

    /*IOServiceAddInterestNotification(gNotifyPort, hidDevice, 
        kIOGeneralInterest, interestCallback, this, &myNotification);*/
    
    ioReturnValue = (*myInterface)->open(myInterface, 0);
    ThrowOnIOError(ioReturnValue, "Failed to open device");
    
    // Get the properties we're interested in
    CFMutableDictionaryRef properties = CreateProperties(hidDevice);
    
    CFStringRef mfg = (CFStringRef) CFDictionaryGetValue(properties, CFSTR(kIOHIDManufacturerKey));
    myManufacturerName = CFToStdLibraryString(mfg);
    CFStringRef dev = (CFStringRef) CFDictionaryGetValue(properties, CFSTR(kIOHIDProductKey));
    myDeviceName = CFToStdLibraryString(dev);    
    CFRelease(properties);
    
    // Get the individual elements (axes, buttons, etc)
    CFArrayRef elements = NULL;
    ioReturnValue = (*myInterface)->copyMatchingElements(myInterface, NULL, &elements);
    ThrowOnIOError(ioReturnValue, "Failed to copyMatchingElements");
    myElementCount = CFArrayGetCount(elements);
    myElements = new HIDElement*[myElementCount];
    for (unsigned int i=0; i<myElementCount; i++)
    {
        CFDictionaryRef element = (CFDictionaryRef) CFArrayGetValueAtIndex(elements, i);
        myElements[i] = new HIDElement(i, myInterface, element);
    }    
}

HIDevice::~HIDevice()
{
    StopQueue();
    for (unsigned int i=0; i<myElementCount; i++)
        delete myElements[i];
    delete [] myElements;
    (*myInterface)->close(myInterface);
    (*myInterface)->Release(myInterface);
}


void                
HIDevice::StartQueue(unsigned int depth)
{
    if (!myEventQueue)
    {
        myEventQueue = (*myInterface)->allocQueue(myInterface);
        IOReturn result = (*myEventQueue)->create(myEventQueue, 0, depth);
        ThrowOnIOError(result, "StartQueue create");
        result = (*myEventQueue)->start(myEventQueue); 
        ThrowOnIOError(result, "StartQueue start");
    }
}

void                
HIDevice::StartMonitoringElement(unsigned int index)
{
    if (!myEventQueue)
        return;
    HIDElement* element = (HIDElement*) GetElement(index);
    if (element)
    {
        IOReturn result = (*myEventQueue)->addElement(myEventQueue, element->myCookie, 0);
        ThrowOnIOError(result, "StartMonitoringElement");
    }
}

void                
HIDevice::StopMonitoringElement(unsigned int index)
{
    if (!myEventQueue)
        return;
    HIDElement* element = (HIDElement*) GetElement(index);
    if (element)
    {
        IOReturn result = (*myEventQueue)->removeElement(myEventQueue, element->myCookie);
        ThrowOnIOError(result, "StopMonitoringElement");
    }
}

void
HIDevice::StopQueue()
{
   if (myEventQueue)
   {
       (*myEventQueue)->dispose(myEventQueue);
       (*myEventQueue)->Release(myEventQueue);       
       myEventQueue = NULL;
   }
}


bool
HIDevice::PollQueue(IHIDElement*& element, int& value, TimeUnits& time)
{
    if (!myEventQueue)
        return false;
    AbsoluteTime zeroTime = {0,0};
    IOHIDEventStruct event;
    IOReturn result = (*myEventQueue)->getNextEvent(myEventQueue, &event, zeroTime, 0);
    if (result==kIOReturnUnderrun)
        return false;
    else ThrowOnIOError(result, "PollQueue");
    element = NULL;
    for (unsigned int i=0; i<myElementCount; i++)
    {
        if (myElements[i]->myCookie==event.elementCookie)
        {
            element = myElements[i];
            break;
        }
    }
    value = event.value;
    // timestamp is a CoreServices AbsoluteTime; this is identical to Mach time,
    // tho Apple notes "This equivalence was previously undocumented, although 
    // it is true on all shipping versions of Mac OS X and is expected to be true 
    // on all future versions." Hope they're right...
    uint64_t tstamp = *((uint64_t*)&event.timestamp);
    uint64_t when = MachTimeToNanos(tstamp-BaseTime);
    // convert to seconds
    time = float(when) / 1000000000.f;
    return true;
}


CFMutableDictionaryRef
HIDevice::CreateProperties(io_object_t hidDevice)
{
	kern_return_t	        result;
	CFMutableDictionaryRef	properties = NULL;
	char					path[512];

	result = IORegistryEntryGetPath(hidDevice, kIOServicePlane, path);
	if (result != KERN_SUCCESS)
	    return NULL;

	//Create a CF dictionary representation of the I/O 
	//Registry entry's properties
	result = IORegistryEntryCreateCFProperties(hidDevice,
											&properties,
											kCFAllocatorDefault,
											kNilOptions);
    return properties;
}

IHIDElement*        
HIDevice::GetElement(unsigned int index)
{
    if (index>=myElementCount) return NULL;
    return myElements[index];
}

const IHIDElement*  
HIDevice::GetElement(unsigned int index) const
{
    if (index>=myElementCount) return NULL;
    return myElements[index];
}


//------------------------------------------------------------------------------------
// HID Manager
// Initialise/Teardown

HIDManager::HIDManager()
{
    ScanDevices();
    BaseTime = mach_absolute_time();
}

HIDManager::~HIDManager()
{
    ClearDevices();
}

// Adding/removing devices

void 
HIDManager::ScanDevices()
{
    ClearDevices();
    
    // Set up a matching dictionary to search the I/O Registry by class
    // name for all HID class devices
    CFMutableDictionaryRef hidMatchDictionary = IOServiceMatching(kIOHIDDeviceKey);
 
    // Now search I/O Registry for matching devices.
    io_iterator_t hidObjectIterator = NULLHandle;
    IOReturn ioReturnValue = IOServiceGetMatchingServices(kIOMasterPortDefault, 
                            hidMatchDictionary, &hidObjectIterator);
 
    bool noMatchingDevices = (ioReturnValue != kIOReturnSuccess) || (!hidObjectIterator);
 
    // If search is unsuccessful, print message and hang.
    if (noMatchingDevices)
    {
        CFShow(CFSTR("No matching HID class devices found\n"));
        return;
    }
 
    // IOServiceGetMatchingServices consumes a reference to the
    // dictionary, so we don't need to release the ref. NULL it out for safety tho:
    hidMatchDictionary = NULL;    
    
	io_object_t	hidDevice = NULLHandle;
	while ((hidDevice = IOIteratorNext(hidObjectIterator)))
	{
    	IHIDevice* device = new HIDevice(hidDevice);
    	IOObjectRelease(hidDevice);
    	myDevices.push_back(device);    	
	}
	IOObjectRelease(hidObjectIterator);
}


void        
HIDManager::ClearDevices()
{
    DeviceList::iterator walk = myDevices.begin();
    while (walk!=myDevices.end())
    {
        IHIDevice* device = *walk++;
        delete device;
    }
    myDevices.clear();
}


// External getters/setters

unsigned int        
HIDManager::GetDeviceCount() const
{
    return myDevices.size();
}


IHIDevice*          
HIDManager::GetDevice(unsigned int index)
{
    if (index>=GetDeviceCount()) return NULL;
    return myDevices[index];
}

const IHIDevice*    
HIDManager::GetDevice(unsigned int index) const
{
    if (index>=GetDeviceCount()) return NULL;
    return myDevices[index];
}
