#pragma once
#include <map>
#include <string>
#include <misc.h>
#include <iostream>

//#define PROFILER_DISABLE
#ifndef _ASSERT
    #define _ASSERT(expr) assert(expr)
#endif
#ifndef FI
        #ifdef WIN32
                #define FI __forceinline
        #else
                #define FI inline
        #endif
#endif
#ifndef UINT
    typedef unsigned int UINT;
#endif

namespace misc {
class Timer
{
public:
    FI void start() { timer_.start(); }
    FI double timeElapsed() { return timer_.elapsed() / 1000.; }
    QElapsedTimer timer_;
};
} // namespace misc

namespace profiler {
	namespace detail {
		struct Item {
			Item() : time(0), count(0), started(false), replaced(NULL) {}
			double getTime() { return time + (started ? timer.timeElapsed() : 0); }
			void start() {
				_ASSERT(!started);
				if (started)
					throw std::runtime_error("Profiler: Item already started");
				count++;
				started = true;
				timer.start();
			}
			double stop() {
				_ASSERT(started);
				if (!started)
					throw std::runtime_error("Profiler: Item not yet started");
				double const t = timer.timeElapsed();
				time += t;
				started = false;
				return t;
			}
			void setReplaced(Item* const r) {
				_ASSERT(r != this);
				if (r == this)
					throw std::runtime_error("Profiler: Item will replace itself");
				replaced = r;
			}
			double time;
                        unsigned int count;
                        bool started;
			Item* replaced;
			misc::Timer timer;
		};
		typedef std::map<std::string, Item> ItemMap;

		struct NamedItemSimple {
			NamedItemSimple(const Item& item, const std::string& name) : name(name), time(item.time), count(item.count) {}
                        FI bool operator<(const NamedItemSimple& i) const { return time > i.time; }
			std::string name;
			double time;
                        unsigned int count;
		};
	} // namespace detail

	struct Profiler
	{
		void start(const std::string& name) {
			mItems[name].start();
		}
		double stop(const std::string& name) {
			return mItems[name].stop();
		}
		void stopAll() {
			for (detail::ItemMap::iterator i = mItems.begin(); i != mItems.end(); i++)
				if (i->second.started)
					i->second.stop();
		}
		void reset() {
			mItems.clear();
		}
		double operator[](const std::string& name) {
			return mItems[name].getTime();
		}
		void output() {
			double t = 0;
			for (detail::ItemMap::const_iterator i = mItems.begin(); i != mItems.end(); i++)
				t += i->second.time;
			for (detail::ItemMap::const_iterator i = mItems.begin(); i != mItems.end(); i++)
				//if (i->second.time/t > 0.15)
					std::cout << /*"[" << i->second.time/t*100 << "%] " <<*/ i->first << ": " << i->second.time << " (" << i->second.count << ")" << std::endl;
		}
	protected:
		detail::ItemMap mItems;
	};

	struct ProfilerExclusive : public Profiler
	{
		struct AutoStop {
			AutoStop(ProfilerExclusive& profiler)
                : profiler_(profiler), started_(profiler.started) {}
            AutoStop(ProfilerExclusive& profiler, const std::string& name, bool const print = false)
                : profiler_(profiler), started_(profiler.started)
            {
                profiler.start(name, print);
            }
			~AutoStop() {
				if (profiler_.started == started_)
					return;
				profiler_._stop(); // TODO: nicht eher n loop?
                                if ((profiler_.started = started_))
					started_->start();
			}
			ProfilerExclusive& profiler_;
			detail::Item* const started_;
		};
		ProfilerExclusive() : started(NULL) {}

		void section(const std::string& name) {
			std::cout << "> " << name << std::endl;
		}
		void start(const std::string& name, bool const print = false) {
			if (print)
				section(name);
#ifndef PROFILER_DISABLE
			if (started)
				started->stop();
			detail::Item& i = mItems[name];
			i.setReplaced(started);
			started = &i;
			i.start();
#endif
		}
		void _stop() {
			_ASSERT(started);
			if (!started)
				throw std::runtime_error("Profiler: No Item started");
			started->stop();
		}
		void stop() {
#ifndef PROFILER_DISABLE
			_stop();
                        if ((started = started->replaced))
				started->start();
#endif
		}
		void switchTo(const std::string& name, bool const print = false) {
			if (print)
				section(name);
#ifndef PROFILER_DISABLE
			_stop();
			detail::Item& i = mItems[name];
			i.setReplaced(started->replaced);
			started = &i;
			i.start();
#endif
		}
		void stopAll() {
			if (!started)
				return;
			started->stop();
			started = NULL;
		}
		void reset() { started = NULL; Profiler::reset(); }

		/*void stop(const std::string& name) {
			//_ASSERT(started && started == 
		}*/
		double operator[](const std::string& name) {
			return mItems[name].getTime();
		}
		double getTimeSum() {
			double t = 0;
			for (detail::ItemMap::const_iterator i = mItems.begin(); i != mItems.end(); i++)
				t += i->second.time;
			return t;
		}
		void output() {
			double t = 0;
			for (detail::ItemMap::const_iterator i = mItems.begin(); i != mItems.end(); i++)
				t += i->second.time;
			std::cout << "took " << t << " seconds" << std::endl;
			std::vector<detail::NamedItemSimple> items;
			items.reserve(mItems.size());
			for (detail::ItemMap::const_iterator i = mItems.begin(); i != mItems.end(); i++)
				items.push_back(detail::NamedItemSimple(i->second, i->first));
			std::sort(items.begin(), items.end());
			for (std::vector<detail::NamedItemSimple>::const_iterator i = items.begin(); i != items.end(); i++)
				std::cout << "[" << i->time/t*100 << "%] " << i->name << ": " << i->time << " (" << i->count << ")" << std::endl;
			//for (UINT i = 0; i < items.size(); i++)
			//	std::cout << "[" << items[i].time/t*100 << "%] " << i->first << ": " << items[i].time << " (" << items[i->second.count << ")" << std::endl;
				//if (i->second.time/t > 0.15)
					//std::cout << "[" << i->second.time/t*100 << "%] " << i->first << ": " << i->second.time << " (" << i->second.count << ")" << std::endl;
		}
		void write(FILE* const out) {
			for (detail::ItemMap::const_iterator i = mItems.begin(); i != mItems.end(); i++)
				fprintf(out, "%s=%f;\n", i->first.c_str(), i->second.time);
		}
//		void write_gnuplot(FILE* const out, std::vector<std::vector<std::string> >& strings, bool const header = false) {
//			detail::ItemMap items = mItems;
//			UINT misc = -1;
//			std::vector<double> times(strings.size()+1);
//			for (UINT i = 0; i < strings.size(); i++) {
//				_ASSERT(strings[i].size());
//				if (strings[i].size() == 1) {
//					_ASSERT(misc == -1);
//					misc = i;
//					continue;
//				}
//				double t = 0;
//				for (UINT j = 1; j < strings[i].size(); j++) {
//					t += items[strings[i][j]].time;
//					items.erase(strings[i][j]);
//				}
//				times[i] = t;
//			}
//			if (misc == -1) {
//				misc = strings.size();
//				strings.push_back(std::vector<std::string>(1, "miscellaneous"));
//			}
//			if (header) {
//				for (UINT i = 0; i < strings.size(); i++) {
//					fprintf(out, "\"%s\"\t", strings[i][0].c_str());
//				}
//				fprintf(out, "\n");
//			}
//			double t = 0;
//			for (detail::ItemMap::const_iterator i = items.begin(); i != items.end(); i++) {
//				t += i->second.time;
//			}
//			times[misc] = t;
//			for (UINT i = 0; i < strings.size(); i++) {
//				fprintf(out, "%.3f\t", times[i]);
//			}
//			fprintf(out, "\n");
//		}
	protected:
		detail::Item* started;
	};

	struct Counter {
#ifdef PROFILER_DISABLE
		FI void operator++() {}
#else
		Counter() : mCounter(0) {}
		FI void operator++() { mCounter++; }
		FI void operator++(int) { mCounter++; }
		FI operator UINT() { return mCounter; }
	protected:
		UINT mCounter;
#endif
	};

} // namespace Profiler
