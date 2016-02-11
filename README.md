//////////////////////////////////////////////////////////////////////////
// little endian
//     HEADER  |<------- packet size[byte] ----------------->|   FOOTER
// +-----------+--------------+------+-----+-------+---------+-----------+-----------+
// | 0x10 0x02 | payload size | 0xFF | CMD | STAT? | payload | 0x10 0x03 | checksum? |
// +-----------+--------------+------+-----+-------+---------+-----------+-----------+
//     2byte       1byte       1byte  1byte  1byte     Xbyte    2byte       1byte
//
// checksum = payload size ^ 0xFF ^ CMD ^ STAT ^ payload ^ 0x03
//
// CMD :
//  0x2b : GetLimitValue(double[6])
//  0x30 : GetLatestData(double[6], char[1])
//  0x32 : SetSerialMode(true)
//  0x33 : SetSerialMode(true)
//
// GetLimitValue :
//   send :
//     SIZE    =  4[byte] = 0x04
//     STAT    = 0x00[byte] expected
//   recv :: 
//     SIZE    = 28[byte] = 0x1c 
//     STAT    = 0x00[byte] expected
//     payload = float64_t * 6 = 24 [byte]
//               Fx,Fy,Fz,Tx,Ty,Tz
//
// GetLatestData :
//   send :
//     SIZE    =  4[byte] = 0x04
//     STAT    = 0x00[byte] expected
//   recv :: 
//     SIZE    = 28[byte] = 0x14
//     STAT    = 0x00[byte] expected
//     payload = int16_t * 8 = 16 [byte]
//               Fx,Fy,Fz,Tx,Ty,Tz,?,?
//
// SetSerialMode(true/false) :
//   send/recv :
//     SIZE    =  4[byte] = 0x04
//     STAT    = 0x00[byte] expected
// 
// GetSerialData :
//   recv :
//     SIZE    =  4[byte] = 0x14
//     STAT    = 0x00[byte] expected
//     payload = int16_t * 8 = 16 [byte]
//               Fx,Fy,Fz,Tx,Ty,Tz,?,?
//

> stty -F /dev/ttyACM0 raw

