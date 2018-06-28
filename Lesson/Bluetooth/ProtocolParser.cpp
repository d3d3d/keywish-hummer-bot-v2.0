#include "Protocol.h"
#include "ProtocolParser.h"
#define DEBUG_LEVEL DEBUG_LEVEL_ERR
#include "debug.h"

ProtocolParser::ProtocolParser(byte startcode = PROTOCOL_START_CODE, byte endcode = PROTOCOL_END_CODE)
{
    m_recv_flag = false;
    m_send_success = false;
    m_StartCode = startcode;
    m_EndCode = endcode;
    m_pHeader = buffer;        // protocol header
    protocol_data_len = 0;     // protocol->data length  
    m_PackageLength = 0;       // recevie all package length  
    m_CheckSum = 0x0000;
    m_RecvDataIndex = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = 0;
    }
}  
  
ProtocolParser::~ProtocolParser()  
{
    m_pHeader = NULL;
}

bool ProtocolParser::ParserPackage(char *data = NULL)
{
    if( data != NULL) {
        m_pHeader = data;
    } else {
        m_pHeader = buffer;
    }
    unsigned short int check_sum = 0;
    recv->start_code = buffer[0];
    for (int i = 1; i < m_PackageLength - 3; i++) {
        check_sum += buffer[i];
    }
    if ((check_sum & 0xFF) != GetCheckSum()) {
         DEBUG_ERR("check sum error \n");
         return false ;
    }
    recv->function = buffer[3];
    recv->data = &buffer[4];
    protocol_data_len = m_PackageLength - 7;
    recv->end_code = buffer[m_RecvDataIndex];
    return true;
    
}

bool ProtocolParser::RecevData(void)
{
    DEBUG_LOG(DEBUG_LEVEL_INFO, "RecevData start \n");
    bool avilable = false;
    m_PackageLength = 0;
    m_RecvDataIndex = 0;
    m_pHeader = buffer;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = 0;
    }
    while (Serial.available() > 0) {
        if(!avilable && Serial.peek() == m_StartCode)  {
            avilable = true;
        }
        if (avilable) {
            if ((*m_pHeader = Serial.read()) == m_EndCode) {
                // Serial.print(*p,HEX);
                DEBUG_LOG(DEBUG_LEVEL_INFO, "%x", *m_pHeader);
                break;
            }
            // Serial.print(*p,HEX);
            DEBUG_LOG(DEBUG_LEVEL_INFO, "%x", *m_pHeader);
            if (m_RecvDataIndex >= BUFFER_SIZE - 1) {
                DEBUG_ERR("buffer is full \n");
                return false;
            }
            m_pHeader++;
            m_RecvDataIndex++;
        }
    }
    if (avilable)
    m_PackageLength =  m_RecvDataIndex + 1;
    DEBUG_LOG(DEBUG_LEVEL_INFO, "\nRecevData done \n");
    return avilable;
}

bool ProtocolParser::RecevData(char *data, size_t len)
{
    DEBUG_LOG(DEBUG_LEVEL_INFO, "RecevPackage start \n");
    bool avilable = false;
    if ( data == NULL || len > BUFFER_SIZE)
    {
        DEBUG_ERR("len > BUFFER_SIZE \n");
        return false;
    }
    m_PackageLength = 0;
    m_pHeader = buffer;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = 0;
    }
    while (len--) {
        if(!avilable && *data == m_StartCode)  {
            avilable = true;
        }
        if (avilable) {
            if ((*m_pHeader = *data) == m_EndCode) {
                m_PackageLength++;
                //Serial.print(*p,HEX);
                DEBUG_LOG(DEBUG_LEVEL_INFO, "%x", *m_pHeader);
                break;
            }
            //Serial.print(*p,HEX);
            DEBUG_LOG(DEBUG_LEVEL_INFO, "%x", *m_pHeader);
            m_pHeader++;
            m_RecvDataIndex++;
        }
        data++;
    }
    if (avilable)
    m_PackageLength =  m_RecvDataIndex + 1;
    DEBUG_LOG(DEBUG_LEVEL_INFO, "\nRecevPackage done \n");
    return true;
}


E_TYPE ProtocolParser::GetRobotType()
{
    return (E_TYPE)recv->type;
}

uint8_t ProtocolParser::GetRobotAddr()
{
    return recv->addr;
}

E_CONTOROL_FUNC ProtocolParser::GetRobotControlFun()
{
    return (E_CONTOROL_FUNC)recv->function;
}

int ProtocolParser::GetRobotSpeed()
{
    if (recv->function == E_ROBOT_CONTROL_SPEED ) {
        return (int)(*(recv->data));
    } else {
        return 0;
    }
}

int ProtocolParser::GetRobotDegree()
{
    if (recv->function == E_ROBOT_CONTROL_DIRECTION ) {
        return ((int)(*(recv->data)<< 8) | (int)(*(recv->data+1)));
    } else {
        return 0;
    }
}

uint8_t ProtocolParser::GetProtocolDataLength()
{
    return protocol_data_len;
}

uint8_t ProtocolParser::GetPackageLength()
{
    return m_PackageLength;
}

uint16_t ProtocolParser::GetCheckSum(void)
{
    
    return ((buffer[m_PackageLength - 3] << 8 ) |  buffer[m_PackageLength - 2]);

}

// len : protocol data length
bool ProtocolParser:: SendPackage(ST_PROTOCOL *send_dat,int len)
{      
    if( send_dat == NULL || len > BUFFER_SIZE) {
        DEBUG_ERR("SendPackage error");
        return false;
    }
    unsigned short checksum = 0;
    byte *p_data = &buffer[4];
    protocol_data_len = len;
    buffer[0] = send_dat->start_code;
    buffer[1] = send_dat->type;
    buffer[2] = send_dat->addr; 
    buffer[3] = send_dat->function;
    checksum = buffer[1] + buffer[2] + buffer[3];
    for(int i = 0; i < len; i++) {
       *(p_data+i) = *(send_dat->data + i);
       checksum += *(send_dat->data + i);
    }
    *(p_data + len) = (checksum >> 8) & 0xFF;
    *(p_data + len + 1) = checksum & 0xFF;
    *(p_data + len + 2) = send_dat->end_code;

    Serial.write(buffer,len);
    Serial.flush();
    return true;
}


