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

    explicit EdmiCMDReader(HardwareSerial &serial, uint8_t rxpin, uint8_t txpin, const char *typeEdmi) : serial_(serial)
    {
        rx_ = rxpin;
        tx_ = txpin;
        typeEdmi_ = typeEdmi;
    }

    EdmiCMDReader(EdmiCMDReader const &) = delete;

    EdmiCMDReader(EdmiCMDReader &&) = delete;
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

    String edmi_R_serialnumber(/*char *output, int len*/);

    Status status() const { return status_; }
    void acknowledge()
    {
        if (status_ != Status::Busy)
            status_ = Status::Ready;
    }

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

    void step_login();
    void step_logout();
    void TX_byte(unsigned char d);
    bool RX_char(unsigned int timeout, byte *inChar);

    uint8_t RX_message(char *message, int maxMessageSize, unsigned int timeout);
    Step step_;
    ErrorCode regError_;
    HardwareSerial &serial_;
    Status status_ = Status::Ready;
    uint8_t rx_, tx_;
    const char *typeEdmi_;
};