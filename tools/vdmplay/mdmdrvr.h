
#ifndef __MDMDRVR_H__
#define __MDMDRVR_H__

#include <vector>

class CCommPort;

class CDataLogger;


class ModemDriver {

public:
    typedef void (*Callback)(void *);

    virtual void setCallback(Callback cbk, void *data);


    enum ResponseCode {
        Nomatch, OK, Error, Nocarrier, Connect, Busy, Noanswer
    };

    struct ModemResponse {
        int code;
        const char *string;
    };

    ModemDriver(CCommPort *p);

    ModemDriver(CCommPort *p, const std::vector<AString> &modemProgram);

    virtual ~ModemDriver();

    void setLogger(CDataLogger *l) { logger = l; }

    void setProgram(const std::vector<AString> &modemProgram);

    void setResponses(const ModemResponse *r, int count) {
        responses = r;
        responseCount = count;
    }

    bool done() const { return progPhase >= (int) prog.size(); }

    bool stillWaiting() const { return waiting; }

    bool getResult() const { return result; }

    void start();

    virtual void commEvent();

    virtual void timerEvent();


protected:

    virtual void timeout();


    void startTiming(unsigned long timeout) {
        endTime = currentTime() + timeout;
    }


    static void portCallback(void *);

    virtual unsigned long currentTime() = 0;

    virtual bool sendNextProgramString();

    virtual int readAndMatch();

    virtual void readFromPort(AString &s);

    virtual void sendToPort(const char *p, int s);

    void send(const char *s) { sendToPort(s, strlen(s)); }

    void send(const AString &s) { sendToPort(s, s.length()); }

    void pushBack();


    void notifyClient();

protected:


    CCommPort *port;
    std::vector<AString> prog;
    int progPhase;
    AString incomingData;
    bool waiting;
    bool result;
    unsigned long endTime;
    const ModemResponse *responses;
    int responseCount;
    Callback callBack;
    void *cbData;
    CDataLogger *logger;

};

#endif

