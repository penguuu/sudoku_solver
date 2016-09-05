/*
 - randomiratkaisu-optio 
*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#define MAX_BOARDS 256
#define VERSION "0.2"
#define MAX_FNAME 256

#define MSG_ERROR 0x01
#define MSG_INFO  0x02
#define MSG_DEBUG 0x04

#define FLAG_STATS 		0x01
#define FLAG_FILE_DEFINED	0x02
#define FLAG_OFILE_DEFINED	0x04
#define FLAG_RANDOM		0x08

typedef struct __stats {
	unsigned int guesses;
	unsigned int backtracks;
	struct timeval start_time;
} _stats;

_stats global_stats;
_stats stats;

short flags = 0;
short show_msgs = 0;

int print_msg(int msg_flag, const char *fmt,...);

int main(int argc, char **argv){
	char f_name[MAX_FNAME];
	char out_file[MAX_FNAME];
	int opt;
	int num;
	int n;
	int k;
	int i_boards[MAX_BOARDS][9][9];

	struct timeval time_now;
	struct timeval global_time_used;
	struct timeval global_time_now;
	
	show_msgs |=MSG_INFO;
	show_msgs |=MSG_ERROR;
	
	while (1) {
		int current_optind = optind ? optind : 1;
		int idx = 0;
		static struct option long_options[] = {
			{"file", required_argument,0,'f'},
			{"output",required_argument,0,'o'},
			{"help",no_argument,0,'h'},
			{"debug",no_argument,0,'d'},
			{"statistics",no_argument,0,'s'},
			{"version",no_argument,0,'v'},
			{"random",no_argument,0,'r'},
			{0,0,0,0 }
		};

		opt = getopt_long(argc, argv,"f:o:hdsvr",
			long_options, &idx);

		if(opt == -1)
			break;

		switch(opt){
			case 'f':
				strncpy(f_name,optarg,MAX_FNAME);
				flags |= FLAG_FILE_DEFINED;	
				break;
			case 'o':
				strncpy(out_file,optarg,MAX_FNAME);
				flags |= FLAG_OFILE_DEFINED;
				break;				
			case 'h':			
				print_msg(MSG_INFO,"Usage: %s [options]\n",argv[0]);
				print_msg(MSG_INFO,"   -d\t--debug              Debug mode\n");
				print_msg(MSG_INFO,"   -f\t--file <file_name>   Reads input from file\n");	
				print_msg(MSG_INFO,"   -h\t--help               Show this help\n");
				print_msg(MSG_INFO,"   -o\t--output <file_name> Prints output to file\n");
				print_msg(MSG_INFO,"   -r\t--random		Random solver instead of linear\n");
				print_msg(MSG_INFO,"   -s\t--statistics         Show statistics from solving\n");
				print_msg(MSG_INFO,"   -v\t--version            Show version and exit\n");
				return 0;
			case 'd':
				show_msgs |= MSG_DEBUG;
				break;
			case 's':
				flags |= FLAG_STATS;
				break;
			case 'r':
				flags |= FLAG_RANDOM;
				break;
			case 'v':
				print_msg(MSG_INFO,"version %s\n",VERSION);
				return 0;
		}
	}

	if(flags&FLAG_FILE_DEFINED){
		num = read_from_file(f_name,i_boards);
		if(num == -1)
			return 0;
	}
	else {		
		print_msg(MSG_ERROR,"Please input a filename with -f option\n");
		return 0;		
	}

	global_stats.guesses = 0;
	global_stats.backtracks = 0;

	gettimeofday(&global_stats.start_time,NULL);

	print_msg(MSG_DEBUG,"Sudokus to be solved: %d\n",num);

	for(n=0;n<num;n++){
		stats.guesses = 0;
		stats.backtracks = 0;
		gettimeofday(&stats.start_time,NULL);

		print_msg(MSG_INFO,"\n*** Solving puzzle number %d ***\n",n+1);
	
		solve(i_boards[n]);

		if(flags&FLAG_OFILE_DEFINED){
			k = write_to_file(out_file,i_boards[n]);
			if(k == -1)
				return 0;
			else 
				print_msg(MSG_DEBUG,"Wrote %d bytes to \"%s\"\n",k,out_file);
		
		}

		global_stats.guesses+=stats.guesses;
		global_stats.backtracks+=stats.backtracks;
	}

	if(flags&FLAG_STATS){
		gettimeofday(&global_time_now,NULL);
		print_msg(MSG_INFO,"\n*** Overall statistics ***\n");
		print_msg(MSG_INFO,"Sudokus solved: %d\n",num);
		print_msg(MSG_INFO,"Overall guesses: %d\n",global_stats.guesses);
		print_msg(MSG_INFO,"Overall backtracks: %d\n",global_stats.backtracks);
	
		substract_time(&global_time_used,&global_stats.start_time,&global_time_now);
		print_msg(MSG_INFO,"Overall time used: %ld.%06ld\n",global_time_used.tv_sec,global_time_used.tv_usec);
		print_msg(MSG_INFO,"***************************\n");
	}
}

int write_to_file(char *filename, int board[9][9]){
	FILE *fd;
	int x=0,y=0;
	int ret = 0;
	unsigned int char_count = 0;

	print_msg(MSG_DEBUG,"write_to_file()\n");

	fd = fopen(filename,"a");

	if(!fd){
		print_msg(MSG_ERROR,"Unable to open file \"%s\" for writing\n",filename);
		return -1;
	}

	for(y=0;y<9;y++){
		for(x=0;x<9;x++){		
			if(board[y][x]>=1 && board[y][x] <= 9){
				ret = fprintf(fd,"%d",board[y][x]);
				if(ret<0){ 
					print_msg(MSG_DEBUG,"fprintf() returned %d\n",ret);
					print_msg(MSG_ERROR,"Error writing to file \"%s\"\n",filename);
				} else { char_count+=ret; }
			}
			else if(board[y][x] == 0){				
				ret = fprintf(fd,".");
				if(ret<0){
					print_msg(MSG_DEBUG,"fprintf() returned %d\n",ret);
					print_msg(MSG_ERROR,"Error writing to file \"%s\"\n",filename);
				} else { char_count+=ret; }
			}
			if(x==2 || x==5){
				ret = fprintf(fd,"|");
				if(ret<0){
					print_msg(MSG_DEBUG,"fprintf() returned %d\n",ret);
					print_msg(MSG_ERROR,"Error writing to file \"%s\"\n",filename);
				} else { char_count+=ret; }
			}
					
		}
		ret = fprintf(fd,"\n");
		if(ret<0){
			print_msg(MSG_DEBUG,"fprintf() returned %d\n",ret);
			print_msg(MSG_ERROR,"Error writing to file \"%s\"\n",filename);
		} else { char_count+=ret; }

		if(y==2 || y == 5){
			ret = fprintf(fd,"-----------\n");
			if(ret<0){
				print_msg(MSG_DEBUG,"fprintf() returned %d\n",ret);
				print_msg(MSG_ERROR,"Error writing to file \"%s\"\n",filename);
			} else { char_count+=ret; }
		}
	}

	ret = fprintf(fd,"\n");

	if(ret<0){
		print_msg(MSG_DEBUG,"fprintf() returned %d\n",ret);
		print_msg(MSG_ERROR,"Error writing to file \"%s\"\n",filename);
	} else { char_count+=ret; }

	fclose(fd);

	return char_count;
}

int read_from_file(char *filename,int boards[MAX_BOARDS][9][9]){
	FILE *fd;
	int n=0;
	int count=0;
	int ret = 0;
	unsigned int lc = 1;

	print_msg(MSG_DEBUG,"read_from_file()\n");

	fd = fopen(filename,"r");

	if(!fd){
		print_msg(MSG_ERROR,"Unable to open file \"%s\" for reading\n",filename);
		return -1;
	}
	
	while(!ret){
		ret = read_sudoku(fd,&lc,boards[count]);
		
		if(ret == 0 || ret==1) count++;
		
		else if(ret == -1){
			print_msg(MSG_ERROR,"Incorrect file format in \"%s\"\n",filename);
			fclose(fd);
			return -1;
		}
	}
	
	if(show_msgs&MSG_DEBUG){ 
		print_msg(MSG_DEBUG,"printing found sudokus:\n");
		for(n=0;n<count;n++){
			printf("*** %d ***\n",n);			
			print_board(&boards[n]);
		}
	}

	if(count==0){
		print_msg(MSG_ERROR,"No sudokus found from file!\n");
		fclose(fd);
		return -1;
	}

	fclose(fd);

	return count;
}

int read_sudoku(FILE *fd,int *lc,int *board){
	int x=0,y=0;
	char chr;
	char last_chr;

	print_msg(MSG_DEBUG,"read_sudoku()\n");
	
	while(chr=fgetc(fd)){
		if(feof(fd)){
			if(y==9) return 1;
			else return 2;
		}

		else if(chr=='\n'){
			*lc=*lc+1;

			if(last_chr == '\n')
				return 0;

			if(last_chr != '-'){
				if(y<9) y++;
				else {
					print_msg(MSG_DEBUG,"\nillegal value for y: %d\n",y);
					print_msg(MSG_ERROR,"format error in line %d\n",*lc);
					return -1;
				}
				x=0;
			}
		}

		else if(chr=='.'){
			*(board+((y*9)+x)) = 0;
			if(x<9) x++;
			else {
				print_msg(MSG_DEBUG,"\nillegal value for x: %d\n",x);
				print_msg(MSG_ERROR,"format error in line %d\n",*lc);
				return -1;
			}
		}

		else if(chr=='|');
		else if(chr=='-');

		else if(chr>=48 && chr<=57){
			*(board+((y*9)+x)) = chr-48;
			if(x<9) x++;
			else {
				print_msg(MSG_DEBUG,"\nillegal value for x: %d\n",x);
				print_msg(MSG_ERROR,"format error in line %d\n",*lc);
				return -1;
			}
		}
		else {
			print_msg(MSG_ERROR,"format error in line %d\n",*lc); 
			print_msg(MSG_ERROR,"unknown character \"%c\"\n",chr);
			return -1;
		}

		print_msg(MSG_DEBUG,"%c",chr);
		last_chr = chr;		
	}
}

int substract_time(struct timeval *res,struct timeval *t1, struct timeval *t2){
	long int d;

	d = (t2->tv_usec + 1000000*t2->tv_sec) - (t1->tv_usec + 1000000*t1->tv_sec);
	res->tv_sec = d/1000000;
	res->tv_usec = d%1000000;

	return 0;
}

int solve(int board[9][9]){
	int n,m,t;
	int u;
	int used[9];
	struct timeval time_used;
	struct timeval time_now;

	for(t=0;t<10;t++){ used[t] = 0; }

	if(check_if_all_known(board) == 1){
		
		print_msg(MSG_DEBUG,"check_if_all_known(board) == 1\n");

		gettimeofday(&time_now,NULL);

		print_board(board);
	
		if(flags&FLAG_STATS){
			print_msg(MSG_INFO,"Guesses: %d\n",stats.guesses);
			print_msg(MSG_INFO,"Backtracks: %d\n",stats.backtracks);
	
			substract_time(&time_used,&stats.start_time,&time_now);
			print_msg(MSG_INFO,"Time used: %ld.%06ld\n",time_used.tv_sec,time_used.tv_usec);
		}
		
		return 1;
	}

	if(flags&FLAG_RANDOM){
		for(n=0;n<9;n++){
			for(m=0;m<9;m++){
				if(board[n][m] == 0){
					for(t=1;u<9;t++){
						board[n][m] = get_rand_number(9,t);
						if(check_used(used,board[n][m]) == 0){
							save_used(used,board[n][m]);
							u++;
							stats.guesses++;
							if(check_sudoku_condition(board)){
								if(solve(board) == 1)
									return 1;
							}
							board[n][m] = 0;
						}
					}
					u=0;for(t=0;t<10;t++){ used[t] = 0; }
					stats.backtracks++;
					return 0;
				}
			}
		}
	}
	else {	
		for(n=0;n<9;n++){
			for(m=0;m<9;m++){
				if(board[n][m] == 0){
					for(t=1;t<=9;t++){
						board[n][m] = t;
						stats.guesses++;
						if(check_sudoku_condition(board)){
							if(solve(board) == 1)
								return 1;
						}
						board[n][m] = 0;
					}
					stats.backtracks++;
					return 0;
				}
			}
		}
	}		
}

int get_rand_number(int n,int k){
	int t;

	srand(time(NULL)/k);
	t = ((int)random()%n);
	t+=1;

	return t;
}

int save_used(int used[],int num){
	int n;
	
	for(n=0;n<10;n++){
		if(used[n] == 0){
			used[n] = num;
			return 1;
		}
	}
	return 0;
}

int check_used(int used[],int num){
	int n;

	for(n=0;n<10;n++){
		if(used[n] == num)
			return 1;
	}
	return 0;
}

int print_msg(int msg_flag, const char *fmt,...){
	int n;
	int size=512;
	char *p,*np;
	va_list ap;

	if(!(show_msgs&msg_flag))
		return 0;		

	if((p=malloc(size)) == NULL)
		return -1;

	while(1){
		va_start(ap,fmt);
		n=vsnprintf(p,size,fmt,ap);
		va_end(ap);
	
		if(n>-1 && n<size){
			printf("%s",p);
			return 0;
		}
	
		if(n>-1) size = n+1;
		else size *=2;

		if((np=realloc(p, size)) == NULL){
			free(p);
			return -1;
		} else {
			p = np;
		}
	}
}

int print_board(int board[9][9]){
	int n,l;

	printf("---\n");
	for(n=0;n<9;n++){
		for(l=0;l<9;l++){
			printf("%d ",board[n][l]);
		}
		printf("\n");
	}
	printf("---\n");
}

int check_if_all_known(int board[9][9]){
	int l,m;

	for(l=0;l<9;l++){
		for(m=0;m<9;m++){
			if(board[l][m] == 0) return 0;
		}
	}
	return 1;
}

// returns 1 if sudoku condition is legal, otherwise 0
int check_sudoku_condition(int board[9][9]){
	if(check_horizontal(board) == 0){
		return 0;
	}

	if(check_vertical(board) == 0){
		return 0;
	}

	if(check_square(board) == 0){
		return 0;	
	}

	return 1;
}

// returns 1 if ok, otherwise 0
int check_horizontal(int board[9][9]){
	int used_nums[9];
	int i,j,n;


	for(i=0;i<9;i++){
		for(n=0;n<9;n++)
			used_nums[n] ^= used_nums[n];

		for(j=0;j<9;j++){
			for(n=0;n<9;n++){
				if(board[i][j] == used_nums[n] &&
					board[i][j] > 0)
					 return 0;								
			}
			used_nums[j] = board[i][j];
		}
	}
	return 1;
}

// returns 1 if ok, otherwise 0
int check_vertical(int board[9][9]){
	int used_nums[9];
	int i,j,n;

	for(i=0;i<9;i++){

		for(n=0;n<9;n++)
			used_nums[n] ^= used_nums[n];

		for(j=0;j<9;j++){
			for(n=0;n<9;n++){
				if(board[j][i] == used_nums[n] &&
					board[j][i] > 0)
					 return 0;					
			}
			used_nums[j] = board[j][i];
		}
	}	
	return 1;
}

// returns 1 if ok, otherise 0
int check_square(int board[9][9]){
	int used_nums[9];

	int i,j,k,n,t;
	int r,l,v=0;

	for(i=0;i<3;i++){
		for(j=0;j<3;j++){
			r=i*3;
			l=j*3;
			v=0;
	
			for(n=0;n<9;n++)
				used_nums[n] ^= used_nums[n];

			for(k=r;k<r+3;k++){
				for(t=l;t<l+3;t++){		
					for(n=0;n<9;n++){
						if(board[k][t] == used_nums[n] &&
							board[k][t] > 0)
								return 0;
					}
					used_nums[v++] = board[k][t];	
				}
			}
		}
	}
	return 1;	
}

