#include <Type/MK10E.h>
#include <Type/MK6N.h>
#include <config.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>

#include <HardwareSerial.h>

typedef uint8_t byte;

class EdmiCMDReader
{
public:
    enum class Status : uint8_t
    {
        Disconnect,
        Connect,
        Ready,
        LoggedIn,
        NotLogin,
        Busy,
        Finish,
        TimeoutError,
        ProtocolError,
        ChecksumError
    };

    const std::map<EdmiCMDReader::Step, std::string> METER_STEP_MAP = {
        {EdmiCMDReader::Status::Disconnect, "Disconnect"},
        {EdmiCMDReader::Status::Connect, "Connect"},
        {EdmiCMDReader::Status::Ready, "Ready"},
        {EdmiCMDReader::Status::LoggedIn, "LoggedIn"},
        {EdmiCMDReader::Status::NotLogin, "NotLogin"},
        {EdmiCMDReader::Status::Busy, "Busy"},
        {EdmiCMDReader::Status::Finish, "Finish"},
        {EdmiCMDReader::Status::TimeoutError, "Err-Timeout"},
        {EdmiCMDReader::Status::ProtocolError, "Err-Prot"},
        {EdmiCMDReader::Status::ChecksumError, "Err-Chk"}};

    explicit EdmiCMDReader(HardwareSerial &serial, uint8_t rxpin, uint8_t txpin, const char *typeEdmi) : serial_(serial)
    {
        rx_ = rxpin;
        tx_ = txpin;
        typeEdmi_ = typeEdmi;
    }

    EdmiCMDReader(EdmiCMDReader const &) = delete;

    EdmiCMDReader(EdmiCMDReader &&) = delete;
    void begin(unsigned long baud);

    void step_trigger();

    void step_login();
    void step_logout();

    void TX_raw(unsigned char d);
    void TX_cmd(unsigned char *cmd, unsigned short len);

    void edmi_R_FUNC(const byte *reg);

    bool read_default();

    String serialNumber();
    float kVarh();
    float kwhWBP();
    float kwhLWBP();
    float kwhTotal();

    String edmi_R_serialnumber(/*char *output, int len*/);

    Status status() const { return status_; }

protected:
    enum class Step : uint8_t;
    enum class ErrorCode : uint8_t;

    struct
    {
        String serialNumber;
        float kVarh;
        float kwhWBP;
        float kwhLWBP;
        float kwhTotal;
    } _currentValues; // Measured values

    void TX_byte(unsigned char d);
    bool RX_char(unsigned int timeout, byte *inChar);
    void stepping();
    uint8_t RX_message(char *message, int maxMessageSize, unsigned int timeout);
    Step step_;
    ErrorCode regError_;
    HardwareSerial &serial_;
    Status status_ = Status::Ready;
    uint8_t rx_, tx_;
    const char *typeEdmi_;
};