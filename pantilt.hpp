#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class PanTilt {

typedef unsigned char BYTE;

void error_message(const char *msg, int data) {
	printf(msg, data);
}

int
set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof (tty));
        if (tcgetattr (fd, &tty) != 0)
        {
                error_message ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                error_message ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

        int _f;
        int _x_step, _y_step;
        int _current_x;
        int _current_y;
public:
        PanTilt(int xStep, int yStep) {
                _x_step = xStep;
                _y_step = yStep;
        	_f = open("/dev/ttyUSB0", O_RDWR|O_NOCTTY|O_SYNC);
                if (_f >= 0) {
                        puts("opened OK");
                }
                else {
                        puts("problems");
                }

        	set_interface_attribs(_f, B2400, 0);
        }
        
        void setX(int x) {
                _current_x = x;
        }
        
        void setY(int y) {
                _current_y = y;
        }

        void send(BYTE cmd1, BYTE cmd2, BYTE data1, BYTE data2) {
                // ssize_t cnt;
                char buff[10];
                buff[0] = 0xff;
                buff[1] = 1;
                buff[2] = cmd1;
                buff[3] = cmd2;
                buff[4] = data1;
                buff[5] = data2;
                buff[6] = (buff[1] + buff[2] + buff[3] + buff[4] + buff[5]) % 100;
                write(_f, buff, 7);
                // cnt = read(_f, buff, 4);
        }
        void left() {
                send(0,4,0,0);
                usleep(_x_step);
                send(0,0,0,0);
                _current_x--;
        }
        void right() {
                send(0,2,0,0);
                usleep(_x_step);
                send(0,0,0,0);
                _current_x++;
        }
        void up() {
                send(0,8,0,0);
                usleep(_y_step);
                send(0,0,0,0);
                _current_y++;
        }
        void down() {
                send(0,16,0,0);
                usleep(_y_step);
                send(0,0,0,0);
                _current_y--;
        }
        void zero() {
                send(0,7,0,0x22);
                sleep(70);
        }
        void panTo(int x) {
                while(x > _current_x)
                        right();
                while(x < _current_x)
                        left();
        }
        void tiltTo(int y) {
                while(y > _current_y)
                        up();
                while(y < _current_y)
                        down();
        }
        int x() const { return _current_x; }
        int y() const { return _current_y; }
        
        ~PanTilt() {
	        close(_f);
        }
};
