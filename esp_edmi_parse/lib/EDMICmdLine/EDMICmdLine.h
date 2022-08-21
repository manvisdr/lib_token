#include <Type/MK10E.h>
#include <Type/MK6N.h>
#include <config.h>

#include <cstddef>
#include <cstdint>
#include <map>

#include <HardwareSerial.h>

typedef uint8_t byte;

class EdmiCMDReader
{
public:
    enum class connStatus : uint8_t
    {
        Disconnect,
        Connect,
    };

    enum class Status : uint8_t
    {
        Ready,
        LoggedIn,
        NotLogin,
        Busy,
        Finish,
        TimeoutError,
        ProtocolError,
        ChecksumError
    };

    const std::map<connStatus, std::string> CONN_STATUS_MAP = {
        {connStatus::Disconnect, "Disconnect"},
        {connStatus::Connect, "Connect"}};

    const std::map<Status, std::string> EDMI_STATUS_MAP = {
        {Status::Ready, "Ready"},
        {Status::LoggedIn, "LoggedIn"},
        {Status::NotLogin, "NotLogin"},
        {Status::Busy, "Busy"},
        {Status::Finish, "Finish"},
        {Status::TimeoutError, "Err-Timeout"},
        {Status::ProtocolError, "Err-Prot"},
        {Status::ChecksumError, "Err-Chk"}};

    explicit EdmiCMDReader(HardwareSerial &serial, uint8_t rxpin, uint8_t txpin, const char *typeEdmi) : serial_(serial)
    {
        rx_ = rxpin;
        tx_ = txpin;
        typeEdmi_ = typeEdmi;
    }

    EdmiCMDReader(EdmiCMDReader const &) = delete;

    EdmiCMDReader(EdmiCMDReader &&) = delete;
    void detectType();
    void begin(unsigned long baud);

    void keepAlive();
    void loop();

    void TX_raw(unsigned char d);
    void TX_cmd(unsigned char *cmd, unsigned short len);

    void edmi_R_FUNC(const byte *reg);

    void step_start();
    bool read_default();

    String serialNumber();
    float kVarh();
    float kwhWBP();
    float kwhLWBP();
    float kwhTotal();

    String read_Serialnumber(/*char *output, int len*/);
    // void setRegister(unsigned char *buf, uint16_t len);

    Status status() const { return status_; }
    connStatus connstatus() const { return connStatus_; }

    std::string printconnStatus()
    {
        connStatus_ = connstatus();
        auto it = CONN_STATUS_MAP.find(connStatus_);
        if (it == CONN_STATUS_MAP.end())
            return "Unknw";
        return it->second;
    }

    std::string printStatus()
    {
        status_ = status();
        auto it = EDMI_STATUS_MAP.find(status_);
        if (it == EDMI_STATUS_MAP.end())
            return "Unknw";
        return it->second;
    }

    void acknowledge()
    {
        if (status_ != Status::Busy)
            status_ = Status::Ready;
    }

protected:
    enum class Step : uint8_t;
    enum class ErrorCode : uint8_t;
    enum class Read : uint8_t;

    struct
    {
        String serialNumber;
        float kVarh;
        float kwhWBP;
        float kwhLWBP;
        float kwhTotal;
    } _currentValues; // Measured values

    void step_login();
    void step_logout();
    void step_read();
    void TX_byte(unsigned char d);
    bool RX_char(unsigned int timeout, byte *inChar);

    uint8_t RX_message(char *message, int maxMessageSize, unsigned int timeout);

    Step step_;
    ErrorCode regError_;
    HardwareSerial &serial_;
    Status status_ = Status::Ready;
    connStatus connStatus_ = connStatus::Disconnect;
    uint8_t rx_, tx_;
    const char *typeEdmi_;
};