ALL:mkfifo_read mkfifo_write

mkfifo_read:mkfifo_read.c
	gcc -o mkfifo_read mkfifo_read.c

mkfifo_write:mkfifo_write.c
	gcc -o mkfifo_write mkfifo_write.c

clean:
	rm -rf mkfifo_read mkfifo_write