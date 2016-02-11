
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef int errno_t;

#define EOK 0

#define ECALL(function) \
  if (function != EOK) return -1

#define CFS_CMD_LIM_VAL (0x2B)
#define CFS_CMD_CUR_VAL (0x30)
#define CFS_CMD_SER_ON  (0x32)
#define CFS_CMD_SER_OFF (0x33)

static errno_t make_checksum (size_t size, uint8_t payload[/*size*/], uint8_t *checksum) {
    //EINVAL_CHECK(NULL, payload);
    //EINVAL_CHECK(NULL, checksum);

    *checksum = 0;
    for (size_t i = 0; i < size; i++) {
        (*checksum) ^= payload[i];
    }
    (*checksum) ^= 0x03;

    return EOK;
}

typedef struct {
  int fd;
} hr_unixio;

errno_t hr_unixio_init (hr_unixio *io) {
  //EVALUE(NULL, io);

  io->fd = -1;

  return EOK;
}

errno_t hr_open (hr_unixio *io, const char *path) {
  //EVALUE(NULL, io);
  //EVALUE(NULL, path);

  //printf("file open : %s\n", path);
  io->fd = open(path, O_RDWR | O_NOCTTY);
  //io->fd = open(path, O_RDWR);
  if (io->fd == -1) {
    //printf("file can not open : %d\n", errno);
    return errno;
  }

  return EOK;
}

errno_t hr_write (hr_unixio *io, void* data, size_t size, size_t *send_size) {
  //EVALUE(NULL, io);
  //EVALUE(0, io->fd);
  //EVALUE(NULL, data);

  *send_size = write(io->fd, data, size);

  return *send_size == -1 ? errno : EOK;
}

errno_t hr_read (hr_unixio *io, void* data, size_t size, size_t *read_size) {
  //EVALUE(NULL, io);
  //EVALUE(0, io->fd);
  //EVALUE(NULL, data);

  *read_size = read(io->fd, data, size);

  return EOK;
}

errno_t hr_close (hr_unixio *io) {
  //EVALUE(NULL, io);
  //EVALUE(0, io->fd);

  if (close(io->fd) == -1) {
    printf("file can not close : %d\n", errno);
    return errno;
  }

  io->fd = -1;
  return EOK;
}

errno_t pkt_read (hr_unixio *io, void* data, size_t size) {
  size_t read_size;
  ECALL(hr_read(io, data, size, &read_size));
  return (read_size == size) ? EOK : -1;
}

const uint8_t header_sig[2] = {0x10, 0x02};
const uint8_t hooter_sig[2] = {0x10, 0x03};

#define CHECKSUM_SIZE (1)
#define PKT_SIZE(size) (sizeof(header_sig) + size + sizeof(hooter_sig) + CHECKSUM_SIZE)
 
errno_t payload_read (hr_unixio *io, void* data, size_t size) {
  //EVALUE(NULL, io);
  //EVALUE(0, io->fd);
  //EVALUE(NULL, data);

  const size_t pkt_size = sizeof(header_sig) + size + sizeof(hooter_sig) + 1/*chcksum*/;


  size_t assigned = 0;
  uint8_t buff[pkt_size];
  uint8_t pkt[pkt_size];
  /* search header signature */
  size_t signature_count = 0;
  size_t read_offset = 0;
  bool isfinished = false;
  while (!isfinished) {
    const size_t buff_size = pkt_size - read_offset;
    //printf("pkt_read : %zd %zd\n", read_offset, buff_size);
    ECALL(pkt_read(io, buff + read_offset, buff_size));  // TODO: pkt_size < read_offset

    for (size_t i = 0; i < pkt_size - 1; i++) {
      if (buff[i] == header_sig[0]) {
        if (buff[i + 1] == header_sig[1]) {
          if (i < pkt_size - 2) {
            size_t assign_size = pkt_size - i;
            memcpy(pkt, buff + i, assign_size);
            assigned += assign_size;
          }
          /* header signature found */
          isfinished = true;
          break;
        }
      
        if (i == pkt_size - 2) {
          if (buff[i + 1] == header_sig[0]) {
            buff[0] = header_sig[0];
            read_offset = 1;
          } else {
            read_offset = 0;
          }
        }
      }
    }
  }

  size_t remain_size = pkt_size - assigned;
  if (remain_size > 0) {
    ECALL(pkt_read(io, buff, remain_size));
    memcpy(pkt + assigned, buff, remain_size);
  }

  /* skipe 0x10 in paload */
  uint8_t pkt_out[pkt_size];
  size_t idx = 0;
  for (size_t i = 0; i < sizeof(header_sig); i++) {
    pkt_out[i] = pkt[idx++];
  }
  bool is_skiped = false; 
  for (size_t i = sizeof(header_sig); i < pkt_size - sizeof(hooter_sig) - 1/*checksum*/;) {
    uint8_t pktd;
    if (idx >= pkt_size) {
      ECALL(pkt_read(io, &pktd, 1));
    } else {
      pktd = pkt[idx++];
    }

    if (pktd == 0x10) {
      if (is_skiped) {
        is_skiped = false;
      } else {
        is_skiped = true;
        continue;
      }
    }
    pkt_out[i++] = pktd;
  }

  /* get hooter signature and checksum */
  const size_t offset2hooter = sizeof(header_sig) + size;
  if (idx >= pkt_size) {
    ECALL(pkt_read(io, pkt_out + offset2hooter, pkt_size - offset2hooter));
  } else {
    memcpy(pkt_out + offset2hooter, pkt + idx, pkt_size - idx);
    if (offset2hooter < idx) {
      ECALL(pkt_read(io, pkt_out + offset2hooter + pkt_size - idx, idx - offset2hooter));
    }
  }

#if 0
  for (size_t i = sizeof(header_sig); i < pkt_size - sizeof(hooter_sig) - 1/*checksum*/; i++) {
    if (pkt[i] == 0x10) {
      if (is_skiped) {
        is_skiped = false;
      } else {
        is_skiped = true;
        continue;
      }
    }
    pkt_out[idx++] = pkt[i];
  }
  for (size_t i = pkt_size - sizeof(hooter_sig) - 1; i < pkt_size; i++) {
    pkt_out[idx++] = pkt[i];
  }
  remain_size = pkt_size - idx;
  if (remain_size > 0) {
    ECALL(pkt_read(io, pkt_out + idx, remain_size));
  }
#endif

  if (pkt_out[offset2hooter] != hooter_sig[0] || pkt_out[offset2hooter + 1] != hooter_sig[1]) {
    printf(" %d %02x %02x\n", __LINE__, pkt_out[offset2hooter], pkt_out[offset2hooter + 1]);
    printf("\n\n\n\n\n\n\n\n");
    return 32;
  }
 
  uint8_t checksum;
  ECALL(make_checksum(size, pkt_out + sizeof(header_sig), &checksum));
  if (checksum != pkt_out[pkt_size - 1]) {
    return 33;
  }
 
  memcpy(data, pkt_out + sizeof(header_sig), size);

  return EOK;
}

errno_t cfs_data_read (hr_unixio *io, void* data, size_t size, uint8_t cmd, uint8_t *status) {
  const size_t payload_head_size = 4;
  const size_t payload_size = size + payload_head_size;
  uint8_t cfs_data[payload_size];

  ECALL(payload_read(io, cfs_data, payload_size));
  if (cfs_data[0] != payload_size) {
    return 40;
  }

  if (cfs_data[1] != 0xFF) {
    return 41;
  }

  if (cfs_data[2] != cmd) {
    return 42;
  }

  *status = cfs_data[3];
  memcpy(data, cfs_data + payload_head_size, size);

  return EOK;
}

errno_t send_pkt (hr_unixio *io, uint8_t cmd) {
  const size_t size = 4;
  const size_t pkt_size = PKT_SIZE(size);
  uint8_t sdata[pkt_size];

  size_t idx = 0;
  for (size_t i = 0; i < sizeof(header_sig); i++) {
    sdata[idx++] = header_sig[i];
  }

  sdata[idx++] = size;
  sdata[idx++] = 0xff;
  sdata[idx++] =  cmd;
  sdata[idx++] = 0x00;

  for (size_t i = 0; i < sizeof(hooter_sig); i++) {
    sdata[idx++] = hooter_sig[i];
  }

  //const size_t offset2checksum = sizeof(header_sig) + size + sizeof(hooter_sig);
  make_checksum(size, sdata + sizeof(header_sig), sdata + idx++); 

  size_t send_size;
  ECALL(hr_write(io, sdata, sizeof(sdata), &send_size));

  return EOK;
}

int main(void) {
  uint8_t size = 0x04;
  uint8_t set_ser_mode_off[] = {0x10, 0x02, 0x04, 0xff, 0x33, 0x00, 0x10, 0x03, 0xcb};
  errno_t eno;

  hr_unixio io;
  hr_unixio_init(&io);
  //eno = hr_open(&io, "/dev/ttyUSB1");
  //eno = hr_open(&io, "/dev/ttyUSB0");
  eno = hr_open(&io, "/dev/ttyACM0");
  //eno = hr_open(&io, "\\\\.\\COM2");
  if (eno != EOK) {
    for (size_t j = 0; j < 10000; j++) {
      printf("00");
      for (size_t i = 0; i < 6; i++) {
        //printf("frc[%1zd] : %04x %lf %+08.4f\n", i, (uint16_t)cur_frcs[i], frcs[i], (cur_frcs[i] - offset_frcs[i]) / 10000.0 * frcs[i]);
        printf(",%+08.4f", (i + 1) / 100.0 * j);
      }
      printf("\n");
      fflush(stdout);
    }
    return 0;
  }

  //printf("start\n");
 
  //size_t send_size;
  //hr_write(&io, set_ser_mode_off, sizeof(set_ser_mode_off), &send_size);
  //printf("send %zd :", send_size);
  //for (size_t i = 0; i < send_size; i++) {
  //  printf(" %02x", set_ser_mode_off[i]);
  //}
  //printf("\n");

  ECALL(send_pkt(&io, CFS_CMD_SER_OFF));

  uint8_t read_buff[256] = {0};
  size_t recv_size;
  ////hr_read(&io, read_buff, sizeof(req_limit_value), &recv_size);
  //for (size_t i = 0; i < 10; i++) {
  //  eno = hr_read(&io, read_buff, 16, &recv_size);
  //  if (eno == EOK) {
  //    printf("recv %zd :", recv_size);
  //    for (size_t i = 0; i < recv_size; i++) {
  //       printf(" %02x", read_buff[i]);
  //    }
  //    printf("\n");
  //    break;
  //  }
  //  printf("-");
  //  usleep(100 * 1000);
  //}
  uint8_t status;
  eno = cfs_data_read(&io, read_buff, 7, CFS_CMD_SER_OFF, &status);

  /////////////////////////////

  //uint8_t req_limit_value[] = {0x10, 0x02, size, 0xff, CFS_CMD_LIM_VAL, 0x00, 0x10, 0x03, 0x00/*invalid*/};
  //make_checksum(size, &(req_limit_value[2]), &(req_limit_value[8]));
  //printf("checksum : %02x \n", req_limit_value[8]);

  //hr_write(&io, req_limit_value, sizeof(req_limit_value), &send_size);
  //printf("send %zd :", send_size);
  //for (size_t i = 0; i < send_size; i++) {
  //  printf(" %02x", req_limit_value[i]);
  //}
  //printf("\n");

  ECALL(send_pkt(&io, CFS_CMD_LIM_VAL));

  //uint8_t read_buff[256] = {0};
  //size_t recv_size;
  float frcs[6] = {0.0f};
  //eno = hr_read(&io, read_buff, 33, &recv_size);
  //eno = cfs_data_read(&io, read_buff + 6, 24, CFS_CMD_LIM_VAL, &status);
  eno = cfs_data_read(&io, frcs, 24, CFS_CMD_LIM_VAL, &status);
  if (eno == EOK) {
    //printf("recv %zd :", recv_size);
    //for (size_t i = 0; i < recv_size; i++) {
    //  printf(" %02x", read_buff[i]);
    //}
    //printf("\n");

    //memcpy(&(frcs[0]), &(read_buff[6 + 0]), sizeof(float));
    //memcpy(&(frcs[1]), &(read_buff[6 + 4]), sizeof(float));
    //memcpy(&(frcs[2]), &(read_buff[6 + 8]), sizeof(float));
    //memcpy(&(frcs[3]), &(read_buff[6 +12]), sizeof(float));
    //memcpy(&(frcs[4]), &(read_buff[6 +16]), sizeof(float));
    //memcpy(&(frcs[5]), &(read_buff[6 +20]), sizeof(float));
    //for (size_t i = 0; i < 6; i++) {
    //  printf("frc[%1zd] : %f\n", i, frcs[i]);
    //}
  }

  for (size_t i = 0; i < 10000; i++) {
    // GetLatestData :
    //   send :
    //     SIZE    =  4[byte] = 0x04
    //     STAT    = 0x00[byte] expected
    //   recv :: 
    //     SIZE    = 28[byte] = 0x14
    //     STAT    = 0x00[byte] expected
    //     payload = int16_t * 8 = 16 [byte]
    //               Fx,Fy,Fz,Tx,Ty,Tz,?,?

    //uint8_t req_latest_value[] = {0x10, 0x02, size, 0xff, CFS_CMD_CUR_VAL, 0x00, 0x10, 0x03, 0x00/*invalid*/};
    //make_checksum(size, &(req_latest_value[2]), &(req_latest_value[8]));

    //hr_write(&io, req_latest_value, sizeof(req_latest_value), &send_size);

    ECALL(send_pkt(&io, CFS_CMD_CUR_VAL));

    //printf("send %zd :", send_size);
    //for (size_t i = 0; i < send_size; i++) {
    //  printf(" %02x", req_limit_value[i]);
    //}
    //printf("\n");

    //eno = hr_read(&io, read_buff, 25, &recv_size);
    static int16_t offset_frcs[8] = {0};
    int16_t cur_frcs[8] = {0};
    int16_t *dest_frcs = (i == 0) ? offset_frcs : cur_frcs;
    eno = cfs_data_read(&io, dest_frcs, 16, CFS_CMD_CUR_VAL, &status);
    //eno = cfs_data_read(&io, read_buff + 6, 16, CFS_CMD_CUR_VAL, &status);
    if (eno == EOK) {
      //static int16_t offset_frcs[6] = {0};
      //int16_t cur_frcs[6] = {0};

      //printf("\033[8A");
 
      //printf("recv %zd :", recv_size);
      //for (size_t i = 0; i < recv_size; i++) {
      //  printf(" %02x", read_buff[i]);
      //}
      //printf("\n");

      //int16_t *dest_frcs = (i == 0) ? offset_frcs : cur_frcs;
      //memcpy(&(dest_frcs[0]), &(read_buff[6 + 0]), sizeof(int16_t));
      //memcpy(&(dest_frcs[1]), &(read_buff[6 + 2]), sizeof(int16_t));
      //memcpy(&(dest_frcs[2]), &(read_buff[6 + 4]), sizeof(int16_t));
      //memcpy(&(dest_frcs[3]), &(read_buff[6 + 6]), sizeof(int16_t));
      //memcpy(&(dest_frcs[4]), &(read_buff[6 + 8]), sizeof(int16_t));
      //memcpy(&(dest_frcs[5]), &(read_buff[6 +10]), sizeof(int16_t));
      //printf("\033[K");
      //printf(" COUNT: %05zd STATUS: %02x\n", i, read_buff[5]);
      printf("%02x", status);
      for (size_t i = 0; i < 6; i++) {
        //printf("frc[%1zd] : %04x %lf %+08.4f\n", i, (uint16_t)cur_frcs[i], frcs[i], (cur_frcs[i] - offset_frcs[i]) / 10000.0 * frcs[i]);
        printf(",%+08.4f", (cur_frcs[i] - offset_frcs[i]) / 10000.0 * frcs[i]);
      }
      printf("\n");
      fflush(stdout);
    }

    //usleep(200 * 1000); // OK
    usleep(60 * 1000);  // OK
    //usleep(50 * 1000);  // NG
    //usleep(40 * 1000);  // NG
    //usleep(30 * 1000);  // NG
  }

  // GetSerialData :
  //   recv :
  //     SIZE    =  4[byte] = 0x14
  //     STAT    = 0x00[byte] expected
  //     payload = int16_t * 8 = 16 [byte]
  //               Fx,Fy,Fz,Tx,Ty,Tz,?,?

  return EOK;
}

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
//     SIZE    = 20[byte] = 0x14
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
