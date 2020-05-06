#ifndef UDP_COMMONS_TYPES_H_
#define UDP_COMMONS_TYPES_H_


typedef enum {
    eUdpResult_Ok = 0,
    eUdpResult_Again = 1,
    eUdpResult_Timeout,
    eUdpResult_Already,
    eUdpResult_Failed,
    eUdpResult_NotImplemented
} UdpResult;


#endif//UDP_COMMONS_TYPES_H_
