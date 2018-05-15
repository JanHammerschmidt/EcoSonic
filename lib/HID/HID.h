#ifndef _HID_H_
#define _HID_H_

#include <vector>
#include <string>

typedef std::string     String;
typedef float           TimeUnits;

class HIDException
{
public:
                                HIDException(const char* reason) : myReason(reason) {}
    virtual                    ~HIDException() {}
    virtual const String        GetReason() const { return myReason; }
protected:
    String                      myReason;
};


class IHIDElement
{
public:
    virtual unsigned int        GetIndex()              const = 0;
    virtual unsigned int        GetUsage()              const = 0;
    virtual unsigned int        GetUsagePage()          const = 0;
    virtual int                 GetValue()              const = 0;
    virtual void                SetValue(int value)           = 0;
    virtual int                 GetMinValue()           const = 0;
    virtual int                 GetMaxValue()           const = 0;
    virtual float               GetNormalisedValue()    const = 0;
    virtual void                SetNormalisedRange(float min, float max) = 0;
    
protected:
                                IHIDElement() {}
    virtual                    ~IHIDElement() {}
    friend class                IHIDevice;
    friend class                HIDManager;
};


class IHIDevice
{
public:    
    virtual const String        GetDeviceName() const = 0;
    virtual const String        GetManufacturerName() const = 0;
    virtual const String        GetDeviceClassName() const = 0;
    virtual unsigned int        GetElementCount() const = 0;
    virtual IHIDElement*        GetElement(unsigned int index) = 0;
    virtual const IHIDElement*  GetElement(unsigned int index) const = 0;
    virtual void                StartQueue(unsigned int depth) = 0;
    virtual void                StartMonitoringElement(unsigned int index) = 0;
    virtual void                StopMonitoringElement(unsigned int index) = 0;
    virtual void                StopQueue() = 0;
    virtual bool                PollQueue(IHIDElement*& element, int& value, TimeUnits& time) = 0;
    
protected:
                                IHIDevice() {}
   virtual                     ~IHIDevice() {}
   
   friend class                 HIDManager;
};


class HIDManager
{
public:
                                HIDManager();
                               ~HIDManager();
    
    void                        ScanDevices();
    unsigned int                GetDeviceCount() const;
    IHIDevice*                  GetDevice(unsigned int index);
    const IHIDevice*            GetDevice(unsigned int index) const;
    
private:
    typedef std::vector<IHIDevice*>   DeviceList;
    
    DeviceList                  myDevices;
    void                        ClearDevices();
};

#endif /* _HID_H_ */
