#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 1671
#define TSIZE 1048576
#define SEED 1159241
#define WINDOW_S 5
#define MAX_WORD 100
#define MAX_COUNT 500

int hash[TSIZE];
double W[MAX_LENGTH][MAX_LENGTH];
int total_num = 0;
int Windex = 0;

typedef struct hashnd {
	char *wd;
	int count;
	int index;
	int over;
	int start;
	struct hashnd *next;
}HASHND;

typedef struct word {
	int num;
	int index;
	double co;
}WORDND;

typedef struct buffer {
	int num;
	int index;
	char *word;
	struct buffer *next;
}BUFFER;

WORDND window[MAX_LENGTH];
int sind, eind;
int sentnum = 0;
int punct_l = -1;
int punct_r = -1;
int punct_tail_l = -1;
int punct_tail_r = -1;
BUFFER *buffer = NULL, *bfptr = NULL;

unsigned int bitwisehash(char *word, int tsize, unsigned int seed);
int scmp(char *s1, char *s2);
HASHND ** inithashtable();
void hashinsert(HASHND **ht, char *w, int start);
int indexinhash(HASHND ** ht, char *w);
int printinghash(HASHND ** ht);
int make_W(HASHND **ht, FILE *fin);
int get_word(char *word, FILE *fin);
void flag_one(int index);
void print_buffer(BUFFER *b);
void insert_buffer(BUFFER **b, int sentnum, int ind, char* wd);

unsigned int bitwisehash(char *word, int tsize, unsigned int seed) {
	char c;
	unsigned int h;
	h = seed;
	for (; (c = *word) != '\0'; word++) h ^= ((h << 5) + c + (h >> 2));
	return (unsigned int)((h & 0x7fffffff) % tsize);
}

int scmp(char *s1, char *s2) { 
	while (*s1 != '\0' && *s1 == *s2) { s1++; s2++; }
	return(*s1 - *s2);
}

HASHND ** inithashtable() {
	int i;
	HASHND **ht;
	ht = (HASHND **)malloc(sizeof(HASHND*) * TSIZE);
	for (i = 0; i < TSIZE; i++) ht[i] = (HASHND *)NULL;
	return ht;
}

void hashinsert(HASHND **ht, char *w, int start) {
	HASHND *htmp, *hprv;
	unsigned int hval = bitwisehash(w, TSIZE, SEED);

	for (hprv = NULL, htmp = ht[hval]; htmp != NULL && scmp(htmp->wd, w) != 0; hprv = htmp, htmp = htmp->next);
	if (htmp == NULL) {
		htmp = (HASHND *)malloc(sizeof(HASHND));
		htmp->wd = (char *)malloc(strlen(w) + 1);
		strcpy(htmp->wd, w);
		htmp->count = 1;
		htmp->index = Windex++;
		htmp->over = 0;
		htmp->start = start;
		htmp->next = NULL;
		if (hprv == NULL)
			ht[hval] = htmp;
		else
			hprv->next = htmp;
	}
	else {
		if (htmp->count > MAX_COUNT){
			if (htmp->over == 0) {
				htmp->over = 1; //�󵵼��� �Ѱ�ġ �̻��� ��� flag = 1 ����(�Ŀ� ��� ���ϴ� �ܾ�)
				flag_one(htmp->index);
			}
		}
		htmp->count++;
	}
	return;
}

int indexinhash(HASHND ** ht, char *w) {
	unsigned int hval = bitwisehash(w, TSIZE, SEED);
	HASHND *htmp, *hprv;
	for (hprv = NULL, htmp = ht[hval]; htmp != NULL && scmp(htmp->wd, w) != 0; hprv = htmp, htmp = htmp->next);
	if (htmp == NULL) {
		printf("There isn't %s in hashtable\n", w);
		return -2;
	}
	else {
		if (htmp->over == 1) {
			return -1;
		}
		return htmp->index;
	}
}

int printinghash(HASHND ** ht) {
	HASHND *htmp;
	int max = 100;
	int count = 0;
	for (int i = 0; i < 1048576; i++) {
		htmp = ht[i];
		if (htmp == NULL) {
			continue;
		}
		for (;htmp != NULL; htmp = htmp->next) {
			if (max < htmp->count) count++;
			printf("(%d, %s, %d)\n", htmp->index, htmp->wd, htmp->count);
		}
	}
	return count;
}

int check_start(HASHND **ht, char *wd) {
	HASHND *htmp;
	unsigned int hval = bitwisehash(wd, TSIZE, SEED);
	for (htmp = ht[hval]; htmp != NULL && scmp(htmp->wd, wd) != 0; htmp = htmp->next);
	if (htmp == NULL) return -1;
	else return htmp->start;
}

/*void init_W() {
	for (int i = 0; i < total_num; i++) {
		for (int j = 0; j < total_num; j++) {
			if (i == j)
				W[i][j] = 1;
			else {
				W[i][j] = 0;
			}
		}
	}
}*/

int make_W(HASHND **ht, FILE *fin) {
	char word[MAX_WORD];
	unsigned int indarr[MAX_LENGTH], w1_ind, w2_ind;
	int ind = 0;

	while (!feof(fin)) {
		int i, j, num;
		double weight, part_w, temp;
		int n = get_word(word, fin);
		//printf("%s\n", word);
		if (n == -1) continue;
		else if (n) {
			for (i = 0; i < ind; i++) {
				w1_ind = indarr[i];
				for (j = i + 1, weight = 1.0 - (0.3/WINDOW_S); j <= i + WINDOW_S && j < ind; j++, weight-=(0.3/WINDOW_S)) {
					w2_ind = indarr[j];
					if (w1_ind == -1 || w2_ind == -1) W[w1_ind][w2_ind] = -1;
					else {
						if (W[w1_ind][w2_ind] != 0) {
							num = (int)W[w1_ind][w2_ind];
							part_w = W[w1_ind][w2_ind] - (double)num;
							temp = (num*part_w + 1 * weight) / (num + 1);
							W[w1_ind][w2_ind] = temp + (num + 1);
						}
						else {
							W[w1_ind][w2_ind] = weight + 1;
						}
					}
				}
			}
			ind = 0;
		}
		else {
			int wind;
			wind = indexinhash(ht, word);
			if (wind == -1) {
				continue;
			}
			indarr[ind++] = wind;
		}
	}

	int temp;
	for (int i = 0; i < MAX_LENGTH; i++) {
		for (int j = 0; j < MAX_LENGTH; j++) {
			temp = (int)W[i][j];
			W[i][j] = W[i][j] - (int)W[i][j];
		}
	}
	return 0;
}

int get_word(char *word, FILE *fin) { // return 1: �ٹٲ�, 0:����ܾ�, -1:[TERMS] �Ǵ� [CURRENCY]
	int i = 0, ch;
	for (; ;) {
		ch = fgetc(fin);
		if (ch == '\r') continue;
		if (i == 0 && ((ch == '\n') || (ch == EOF))) {
			word[i] = 0;
			return 1;
		}
		if (i == 0 && ((ch == ' ' || ch == '\t'))) continue;
		if ((ch == EOF) || (ch == ' ') || (ch == '\t') || (ch == '\n')) {
			if (ch == '\n') ungetc(ch, fin);
			break;
		}
		word[i++] = ch;
	}
	word[i] = '\0';
	if (scmp(word, "[TERMS]") == 0 || scmp(word, "[CURRENCY]") == 0) return -1;
	return 0;
}

void flag_one(int index) {
	for (int i = 0; i < total_num; i++) {
		W[index][total_num] = -1;
		W[total_num][index] = -1;
	}
}

void insert_buffer(BUFFER **b, int sentnum, int ind, char* wd) {
	BUFFER *temp, *prev;
	BUFFER *new = (BUFFER*)malloc(sizeof(BUFFER));
	new->word = (char *)malloc(strlen(wd) + 1);
	new->next = NULL; new->num = sentnum; new->index = ind; 
	strcpy(new->word, wd);
	if (*b == NULL) *b = new;
	else {
		for (temp = *b; temp != NULL; prev = temp, temp = temp->next);
		prev->next = new;
	}
}

void print_buffer(BUFFER *b) {
	if (b == NULL) printf("null!\n");
	else {
		BUFFER *temp;
		for (temp = b; temp != NULL; temp = temp->next) {
			printf("%s->", temp->word);
		}
	}
}

void delete_buffer(BUFFER **b) {
	BUFFER *temp;
	temp = *b;
	if (*b != NULL) {
		printf("b->word: %s\n", temp->word);
		BUFFER *result;
		result = *b;
		*b = (*b)->next;
		bfptr = *b;
	}
}

char* printword(BUFFER* b, int punctnum) {
	if (b == NULL) {
		return NULL;
	}
	printf("b->num: %d | sentnum: %d\n", b->num, punctnum);
	printf("<b> num: %d / index: %d / word: %s\n", b->num, b->index, b->word);
	char* word = (char*)malloc(strlen(b->word) + 1);
	if (b->num < punctnum) {
		strcpy(word, b->word);
		return word;
	}
	else return NULL;
}

int main(void)
{
	HASHND **hashtb = inithashtable();
	char word[100];
	int strcount = 0;
	int start = 1;

	//hashtable�� �ܾ �󵵼�, W������ index ����
	FILE *fin = fopen("replace_ordinal.txt", "r");
	while (!feof(fin)) {
		int n = get_word(word, fin);
		if (n == 1) {
			strcount++;
			start = 1;
			continue;
		}
		hashinsert(hashtb, word, start);
		start = 0;
	}
	total_num = Windex;
	fclose(fin);

	//printf("words number%d\n", total_num);

	//printf("max: %d\n", printinghash(hashtb));
	//printf("string number = %d\n", strcount);

	fin = fopen("replace_ordinal.txt", "r");
	if (make_W(hashtb, fin)) {
		printf("error occured in making_W\n");
		return 0;
	}
	fclose(fin);
	/*
	FILE *fout = fopen("matrixW.txt", "w");
	for (int i = 0; i < total_num; i++) {
		for (int j = 0; j < total_num; j++) {
			if (j == total_num - 1) fprintf(fout, "%f", W[i][j]);
			else fprintf(fout, "%f ", W[i][j]);
		}
		fprintf(fout, "\n");
	}
	printf("matrixW.txt genteration completed.\n");
	// cooccurence times + cooccurence probability
	fclose(fout);

	int w;
	FILE *fout2 = fopen("matrix_co.txt", "w");
	for (int i = 0; i < total_num; i++) {
		for (int j = 0; j < total_num; j++) {
			w = (int)W[i][j];
			if (j == total_num - 1) fprintf(fout, "%f", W[i][j] - (double)w);
			else fprintf(fout, "%f ", W[i][j] - (double)w);
		}
		fprintf(fout, "\n");
	}
	printf("matrix_co.txt generation completed.\n");
	// only probability
	fclose(fout);

	FILE *hashfile = fopen("hashtable.txt", "w");
	HASHND *htmp, *hnext;
	for (int i = 0; i < TSIZE; i++) {
		htmp = hashtb[i];
		if (htmp == NULL) continue;
		hnext = htmp->next;
		for (;htmp != NULL; htmp = htmp->next) {
			fprintf(hashfile, "%s %d %d %d\n", htmp->wd, htmp->count, htmp->index, htmp->over);
			if (hnext != NULL) {
				hnext = hnext->next;
			}
			else fprintf(hashfile, "\n");
		}
	}
	printf("hashtable.txt generation completed.\n");
	fclose(hashfile);
	*/
	
	FILE *test = fopen("test_input.txt", "r");
	FILE *result = fopen("test_result.txt", "w");
	int flag1 = 0; //0:ó�� ���� �ޱ� -> WINDOW + 1�� ä������ flag =>1
	int flag2 = 0;
	int prflag;
	int center = WINDOW_S;
	int tmpind;
	int n;

	while (1) {
		/*window update*/
		printf("flag1 = %d | flag2 = %d\n", flag1, flag2);
		if (flag1 == 0) { //window�� center���� �ܾ �ִ� ���
			sind = center; eind = WINDOW_S * 2;
			if (flag2 == 0) { //input stream���κ��� �ܾ �޾� window�� �ִ� ���
				for (int i = sind; i <= eind; i++) {
					if (!feof(test)) {
						n = get_word(word, test);
						tmpind = indexinhash(hashtb, word);
						insert_buffer(&buffer, sentnum, tmpind, word);
						//print_buffer(buffer);
						if (n == -1 || tmpind == -1) {
							sentnum++;
							i--;
							continue;
						}
						window[i].num = sentnum; window[i].index = tmpind;
						sentnum++;
					}
					else {
						eind = i-1;
						break;
					}
				}
				flag2 = 1;
			}
			else { //flag2==1, buffer�κ��� �ܾ �޾� window�� �ִ� ���
				printf("buffer���� �ܾ� ��������\n");
				BUFFER* temp = bfptr;
				int i, index, sentnum;
				for (i = sind; i <= eind; i++) {
					if (temp != NULL && (scmp(temp->word, "[TERMS]") == 0 || scmp(temp->word, "[CURRENCY]") == 0)) {
						i--;
						temp = temp->next;
						continue;
					}
					else if (temp != NULL && temp->index == -1) {
						i--;
						temp = temp->next;
						continue;
					}
					else if (temp != NULL) {
						index = temp->index; sentnum = temp->num;
						window[i].index = index; window[i].num = sentnum;
						temp = temp->next;
					}
					else {
						if (!feof(test)) {
							n = get_word(word, test);
							tmpind = indexinhash(hashtb, word);
							insert_buffer(&buffer, sentnum, tmpind, word);
							//print_buffer(buffer);
							if (n == -1 || tmpind == -1) {
								sentnum++;
								i--;
								continue;
							}
							window[i].num = sentnum; window[i].index = tmpind;
							sentnum++;
						}
						else {
							eind = i - 1;
							break;
						}
					}
				}
				bfptr = temp; //buffer���� ������ ���� ��
			}
			flag1 = 1;
		}
		else { //�� ���ھ�
			if (sind > 0) sind--;
			for (int i = sind; i < eind; i++) {
				window[i].num = window[i + 1].num;
				window[i].index = window[i + 1].index;
			}
			int flag = 0;
			if (bfptr != NULL) { //buffer���� ������ �� �ܾ �߰�����.
				while (bfptr != NULL) {
					if (bfptr->index == -1) {
						bfptr = bfptr->next;
						continue;
					}
					else if (scmp(bfptr->word, "[TERMS]") == 0 || scmp(bfptr->word, "[CURRENCY]") == 0) {
						bfptr = bfptr->next;
						continue;
					}
					window[eind].num = bfptr->num; window[eind].index = bfptr->index;
					bfptr = bfptr->next;
					flag = 1;
					break;
				}
			}
			if (bfptr == NULL && flag == 0) { //input stream���� �� �ܾ ������ �߰�����.
				while (1) {
					if (!feof(test)) {
						n = get_word(word, test);
						tmpind = indexinhash(hashtb, word);
						insert_buffer(&buffer, sentnum, tmpind, word);
						if (n == -1 || tmpind == -1) {
							sentnum++;
							continue;
						}
						window[eind].num = sentnum; window[eind].index = tmpind;
						sentnum++;
						break;
					}
					else {
						eind--; 
						break;
					}
				}
			}
		}

		/*window ���� �ܾ cooccurrence üũ*/
		for (int i = sind; i <= eind; i++) {
			if (i < center) window[i].co = W[window[i].index][window[center].index];
			else if (i == center) window[i].co = 1;
			else window[i].co = W[window[center].index][window[i].index];
		}

		printf("\nsentnum | index | co\n");
		for (int i = sind; i <= eind; i++) {
			printf("%3d: %10d | %10d | %10f\n", i, window[i].num, window[i].index, window[i].co);
		}
		for (int l = center; l > sind; l--) {
			if (window[l - 1].co == 0 && window[l].co > 0)
				if (punct_l == -1) {
					punct_l = window[l].num;
					punct_tail_l = window[l - 1].num;
				}
				else if (punct_l < window[l].num) {
					punct_l = window[l].num;
					punct_tail_l = window[l - 1].num;
				}
		}
		for (int r = center; r < eind; r++) {
			if (window[r].co > 0 && window[r + 1].co == 0)
				if (punct_r == -1) {
					punct_r = window[r + 1].num;
					punct_tail_r = window[r].num;
				}
				else if (punct_r > window[r + 1].num) {
					punct_r = window[r + 1].num;
					punct_tail_r = window[r].num;
				}
		}
		printf("punct_l = %d, punct_r = %d\n", punct_l, punct_r);
		char* outw;
		int f = 0;
		int cnt = 0;
		int breakflag = 0;
		if(eind == 0){
			punct_l = sentnum;
			while (1) {
				outw = printword(buffer, punct_l);
				if (outw != NULL) {
					delete_buffer(&buffer);
					if (f == 0) {
						fprintf(result, "%s", outw);
						f = 1;
					}
					else fprintf(result, " %s", outw);
				}
				else {
					fprintf(result, "\n");
					fclose(test);
					fclose(result);
					return 0;
				}
			}
		}
		else if (punct_l == -1 && punct_r == -1) continue;
		else if ((punct_l < window[sind].num && punct_r < window[sind].num) && (punct_l != -1 && punct_r != -1)) {
			int low, high;
			int tail;
			if (punct_l < punct_r) {
				low = punct_l; high = punct_r;
				tail = punct_tail_l;
			}
			else {
				low = punct_r; high = punct_l;
				tail = punct_tail_r;
			}
			while(1){
				outw = printword(buffer, tail+1);
				if (outw != NULL) {
					delete_buffer(&buffer);
					if (f == 0) {
						fprintf(result, "%s", outw);
						f = 1;
					}
					else fprintf(result, " %s", outw);
				}
				else {
					while (1) {
						if (check_start(hashtb, buffer->word) == 1) {
							fprintf(result, "\n");
							if (bfptr != NULL) flag1 = 0;
							punct_l = -1; punct_r = -1;
							breakflag = 1;
							break;
						}
						else {
							if (buffer->num == high) {
								fprintf(result, "\n");
								if (bfptr != NULL) flag1 = 0;
								punct_l = -1; punct_r = -1;
								breakflag = 1;
								break;
							}
							else {
								delete_buffer(&buffer);
								fprintf(result, " %s", outw);
							}
						}
					}
					if (breakflag == 1) break;
				}
			}
		}
		else if (punct_l == punct_r) {
			int tail = punct_tail_l;
			printf("\n-------------------------------------------\n");
			printf("punct_l is equal to punct_r\n");
			while (1) {
				if (buffer->num > tail) {
					if (check_start(hashtb, buffer->word) == 1) {
						if (bfptr != NULL) flag1 = 0;
						punct_l = -1; punct_r = -1;
						printf("\n\n-----------------------------------------------\n");
						printf("next sentence segmentation!\n\n");
						fprintf(result, "\n");
						break;
					}
				}
				outw = printword(buffer, tail + 1);
				if (outw != NULL) {
					delete_buffer(&buffer);
					if (f == 0) {
						fprintf(result, "%s", outw);
						f = 1;
					}
					else fprintf(result, " %s", outw);
				}
				else {
					if(bfptr != NULL) flag1 = 0;
					punct_l = -1; punct_r = -1;
					printf("\n\n-----------------------------------------------\n");
					printf("next sentence segmentation!\n\n");
					fprintf(result, "\n");
					break;
				}
			}
		}
	}
	fclose(test);
	fclose(result);
	return 0;
}