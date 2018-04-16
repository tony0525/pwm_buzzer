#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

unsigned long beep;
int buzzer_fd;
int button_fd;

int main()
{	
	char button[1];
	int  ret;	
	buzzer_fd = open("/dev/pwm_buzzer", O_WRONLY);
	button_fd = open("/dev/lab5_1", O_RDONLY);
	if(buzzer_fd == -1)
	{
		printf("Fail to open device pwm_buzzer!\n");
		goto finish;	
	}
	if(button_fd == -1)
	{
		printf("Fail to open device lab5_1!\n");
		goto finish;	
	}
	while(1)
	{
		ret = read(button_fd, button, sizeof(button) );
		if (ret != sizeof(button) ) {
			printf("Read key error:\n");
			goto finish;
		}
		if(button[0] == '1'){
			beep = 1000;
		}
		else if(button[0] == '2'){
			beep = 1200;
		} 
		else if(button[0] == '3'){
			beep = 1400;
		} 
		else if(button[0] == '4'){
			beep = 0;
		} 
		write(buzzer_fd, &beep, sizeof(unsigned long));
		printf("The buzzer sound with a frequency of %d Hz\n", beep);
	}
	close(buzzer_fd);
	close(button_fd);

finish:	
	return 0;
}