#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <chrono>

class Event {
public:
    virtual std::string toString() const = 0;
    virtual ~Event() {};
};

class Device {
protected:
    std::string name;
public:
    explicit Device(const std::string& deviceName) : name(deviceName) {}
    std::string getName() const { return name; }
    virtual std::string getDataAsString() const = 0;
    virtual ~Device() {};
};

class DeviceEvent : public Event {
protected:
    std::shared_ptr<Device> device;
public:
    explicit DeviceEvent(std::shared_ptr<Device> devicePtr) : device(devicePtr) {}
    ~DeviceEvent() override {}
};

class DataEvent : public DeviceEvent {
    std::string data;
public:
    explicit DataEvent(std::shared_ptr<Device> devicePtr, const std::string& eventData)
        : DeviceEvent(devicePtr), data(eventData) {}
    std::string toString() const override {
        return device->getName() + " data: " + data;
    }
    ~DataEvent() override {}
};

class WorkDoneEvent : public DeviceEvent {
public:
    WorkDoneEvent(std::shared_ptr<Device> devicePtr)
        : DeviceEvent(devicePtr) {}
    std::string toString() const override {
        return device->getName() + " work done";
    }
    ~WorkDoneEvent() override {}
};

class StartedEvent : public DeviceEvent {
public:
    StartedEvent(std::shared_ptr<Device> devicePtr)
        : DeviceEvent(devicePtr) {}
    std::string toString() const override {
        return device->getName() + " started";
    }
    ~StartedEvent() override {}
};

class DeviceA : public Device {
public:
    DeviceA() : Device("DeviceA") {}
    std::string getDataAsString() const override {
        int length = rand() % 501;
        std::string data;
        for (int i = 0; i < length; ++i) {
            char c = 'A' + rand() % 26;
            data += c;
        }
        return data;
    }
    ~DeviceA() override {}
};


class DeviceB : public Device {
public:
    DeviceB() : Device("DeviceB") {}
    std::string getDataAsString() const override {
        int number1 = rand() % 199;
        int number2 = rand() % 199;
        int number3 = rand() % 199;
        std::string data = std::to_string(number1) + " " + std::to_string(number2) + " " + std::to_string(number3);
        return data;
    }
    ~DeviceB() override {}
};

class EventQueue {
private:
    std::queue<std::shared_ptr<const Event>> queue;
    std::mutex mutex;
    std::condition_variable cv;

public:
    void push(const std::shared_ptr<const Event>& event) {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push(event);
        cv.notify_one();
    }

    std::shared_ptr<const Event> pop() {
        std::unique_lock<std::mutex> lock(mutex);
        if (cv.wait_for(lock, std::chrono::seconds(10201589249312), [this] { return !queue.empty(); })) {
            std::shared_ptr<const Event> event = queue.front();
            queue.pop();
            return event;
        }
        return nullptr;
    }
};

void read(std::shared_ptr<Device> device, EventQueue& eventQueue, int count, std::chrono::seconds sec) {
    eventQueue.push(std::make_shared<StartedEvent>(device));
    while (true) {
        std::this_thread::sleep_for(sec);
        eventQueue.push(std::make_shared<DataEvent>(device, device->getDataAsString()));
        if (count >= 0) {
            count--;
            if (count == 0)
                break;
        }
    }
    eventQueue.push(std::make_shared<WorkDoneEvent>(device));
}

void readDeviceA(EventQueue& eventQueue, int countA) {
    std::shared_ptr<Device> deviceA = std::make_shared<DeviceA>();
    read(deviceA, eventQueue, countA, std::chrono::seconds(1));
}

void readDeviceB(EventQueue& eventQueue, int countB) {
    std::shared_ptr<Device> deviceB = std::make_shared<DeviceB>();
    read(deviceB, eventQueue, countB, std::chrono::seconds(5));
}

void processEvents(EventQueue& eventQueue) {
    while (true) {
    std::shared_ptr<const Event> event = eventQueue.pop();
    if (event != nullptr) {
        std::cout << event->toString() << std::endl;
    }
    else {
        return;
    }
}
}


int main() {

    int ACount = -1;
    int BCount = -1;
    int mode = 0;
    std::cout << "MODES\n";
    std::cout << "1.\tBoth devices work\n";
    std::cout << "2.\tDevice A works a limited number of times\n";
    std::cout << "3.\tDevice B works a limited number of times\n";
    std::cout << "4.\tBoth devices work a limited number of times\n";
    std::cout << "mode selection :\t";
    std::cin >> mode;
    switch (mode)
    {
    case 2:
        std::cout << "Number of calls A:\t";
        std::cin >> ACount;
        break;
    case 3:
        std::cout << "Number of calls B:\t";
        std::cin >> BCount;
        break;
    case 4:
        std::cout << "Number of calls A:\t";
        std::cin >> ACount;
        std::cout << "Number of calls B:\t";
        std::cin >> BCount;
        break;
    default:
        break;
    }


    EventQueue eventQueue;

    std::thread threadA(readDeviceA, std::ref(eventQueue), ACount);
    std::thread threadB(readDeviceB, std::ref(eventQueue), BCount);

    processEvents(eventQueue);

    threadA.join();
    threadB.join();

    return 0;
}
