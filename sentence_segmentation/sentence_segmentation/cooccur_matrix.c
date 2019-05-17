#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 1671
#define TSIZE 1048576
#define SEED 1159241
#define WINDOW_S 5
#define MAX_WORD 100

int hash[TSIZE];
double W[MAX_LENGTH][MAX_LENGTH];
int max_count = 100000;
int total_num = 0;
int Windex = 0;

typedef struct hashnd {
	char *wd;
	int count;
	int index;
	int flag;
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
	char* word;
	struct buffer *next;
}BUFFER;

WORDND window[MAX_LENGTH];
int sind, eind;
int sentnum = 0;
int punct_l = -1;
int punct_r = -1;

unsigned int bitwisehash(char *word, int tsize, unsigned int seed);
int scmp(char *s1, char *s2);
HASHND ** inithashtable();
void hashinsert(HASHND **ht, char *w);
int indexinhash(HASHND ** ht, char *w);
void printinghash(HASHND ** ht);
int make_W(HASHND **ht, FILE *fin);
int get_word(char *word, FILE *fin);
void flag_one(int index);

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

void hashinsert(HASHND **ht, char *w) {
	HASHND *htmp, *hprv;
	unsigned int hval = bitwisehash(w, TSIZE, SEED);

	for (hprv = NULL, htmp = ht[hval]; htmp != NULL && scmp(htmp->wd, w) != 0; hprv = htmp, htmp = htmp->next);
	if (htmp == NULL) {
		htmp = (HASHND *)malloc(sizeof(HASHND));
		htmp->wd = (char *)malloc(strlen(w) + 1);
		strcpy(htmp->wd, w);
		htmp->count = 1;
		htmp->index = Windex++;
		htmp->next = NULL;
		if (hprv == NULL)
			ht[hval] = htmp;
		else
			hprv->next = htmp;
	}
	else {
		if (htmp->count > max_count) {
			if (htmp->flag == 0) {
				htmp->flag = 1; //빈도수가 한계치 이상일 경우 flag = 1 설정(후에 고려 안하는 단어)
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
		if (htmp->flag == 1) {
			return -1;
		}
		return htmp->index;
	}
}

void printinghash(HASHND ** ht, int ind) {
	HASHND *htmp;
	int index = 0;
	int total = 0;
	for (int i = 0; i < 1048576; i++) {
		htmp = ht[i];
		if (htmp == NULL) {
			continue;
		}
		for (;htmp != NULL; htmp = htmp->next) {
			if (htmp->index == ind) {
				printf("(%d, %s, %d)\n", htmp->index, htmp->wd, htmp->count);
				total += htmp->count;
			}
		}
	}
	//printf("total=%d", total);
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
		if (n) {
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
				printf("there is no word in the hashtable!\n");
				return 1;
			}
			indarr[ind++] = wind;
		}
	}
	return 0;
}

int get_word(char *word, FILE *fin) {
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
	return 0;
}

void flag_one(int index) {
	for (int i = 0; i < total_num; i++) {
		W[index][total_num] = -1;
		W[total_num][index] = -1;
	}
}

void insert_buffer(BUFFER *b, int sentnum, int ind, char* wd) {
	BUFFER *new = (BUFFER*)malloc(sizeof(BUFFER));
	new->next = NULL; new->num = sentnum; new->index = ind; new->word = wd;
	BUFFER *temp, *prev = NULL;
	for (temp = b; temp != NULL; prev = temp, temp = temp->next);
	prev->next = new;
}

BUFFER* delete_buffer(BUFFER *b) {
	BUFFER *result;
	if (b != NULL) {
		result = b;
		b = b->next;
	}
	return result;
}

char* printword(BUFFER* b,int sentnum) {
	if (b == NULL) return NULL;
	char* word;
	if (b->num < sentnum) {
		word = b->word;
		delete_buffer(b);
		return word;
	}
	else return NULL;
}

int main(void)
{
	HASHND **hashtb = inithashtable();
	char word[100];

	//hashtable에 단어별 빈도수, W에서의 index 삽입
	FILE *fin = fopen("replace_ordinal.txt", "r");
	while (!feof(fin)) {
		int n = get_word(word, fin);
		if (n) continue;
		hashinsert(hashtb, word);
	}
	total_num = Windex;
	fclose(fin);

	printf("words number%d\n", total_num);

	//printinghash(hashtb);

	int count = 0;
	fin = fopen("replace_ordinal.txt", "r");
	while (!feof(fin)) {
		if (count > 200) break;
		count++;
		int ch = fgetc(fin);
		printf("%c", ch);
	}
	fclose(fin);

	for (int i = 0; i < 5; i++) {
		printinghash(hashtb, i);
	}


	fin = fopen("replace_ordinal.txt", "r");
	if (make_W(hashtb, fin)) {
		printf("error occured in making_W\n");
		return 0;
	}
	fclose(fin);

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
			fprintf(hashfile, "%s %d %d %d\n", htmp->wd, htmp->count, htmp->index, htmp->flag);
			if (hnext != NULL) {
				hnext = hnext->next;
			}
			else fprintf(hashfile, "\n");
		}
	}
	printf("hashtable.txt generation completed.\n");
	fclose(hashfile);


	FILE *test = fopen("test_input.txt", "r");
	FILE *result = fopen("test_result.txt", "w");
	int flag1 = 0; //0:처음 문장 받기 -> WINDOW + 1이 채워지면 flag =>1
	int flag2 = 0;
	int prflag;
	int center = WINDOW_S;
	int tmpind;
	BUFFER *buffer = NULL, *bfptr = NULL;
	sind = center; eind = WINDOW_S * 2;

	while (1) {
		/*window update*/
		if (flag1 == 0) { //window의 center부터 단어를 넣는 경우
			sind = center;
			if (flag2 == 0) { //input stream으로부터 단어를 받아 window에 넣는 경우
				for (int i = sind; i <= eind; i++) {
					if (!feof(test)) {
						get_word(word, test);
						tmpind = indexinhash(hashtb, word);
						insert_buffer(buffer, sentnum, tmpind, word);
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
			else { //flag2==1, buffer로부터 단어를 받아 window에 넣는 경우
				BUFFER* temp = buffer;
				for (int i = sind; i <= eind; i++, temp = temp->next) {
					int index, sentnum;
					if (buffer == NULL) {
						eind = i - 1;
						break;
					}
					index = temp->index; sentnum = temp->num;
					window[i].index = index; window[i].num = sentnum;
				}
				bfptr = temp; //buffer에서 다음에 읽을 값
			}
			flag1 = 1;
		}
		else {
			if (sind > 0) sind--;
			for (int i = sind; i < eind; i++) {
				window[i].index = window[i + 1].index;
				window[i].num = window[i + 1].num;
			}
			if (bfptr == NULL) { //input stream에서 한 단어를 가져와 추가해줌.
				if (!feof(test)) {
					get_word(word, test);
					tmpind = indexinhash(hashtb, word);
					insert_buffer(buffer, sentnum, tmpind, word);
					window[eind].num = sentnum; window[eind].index = tmpind;
					sentnum++;
				}
				else eind--;
			}
			else { //buffer에서 가져와 한 단어를 추가해줌.
				window[eind].num = bfptr->num; window[eind].index = bfptr->index;
				bfptr = bfptr->next;

			}
		}
		/*window 내의 단어간 cooccurrence 체크*/
		for (int i = sind; i < eind; i++) {
			if (i < center) window[i].co = W[window[i].index][window[center].index];
			else if (i == center) window[i].co = 1;
			else window[i].co = W[window[center].index][window[i].index];
		}
		for (int l = center; l > sind; l--) {
			if (window[l].co > 0 && window[l - 1].co == 0)
				if (punct_l < window[l].num) punct_l = window[l].num;
		}
		for (int r = center; r < eind; r++) {
			if (window[r].co > 0 && window[r + 1].co == 0)
				if (punct_r > window[r + 1].num) punct_r = window[r + 1].num;
		}
		if (punct_l == punct_r) {
			char* outw;
			char f = 0;
			while (1) {
				outw = printword(buffer, punct_l);
				if (!outw) {
					if (!f) {
						fprintf(result, "%s", outw);
						f = 1;
					}
					else fprintf(result, " %s", outw);
				}
				else {
					fprintf(result, "\n");
					flag1 = 0;
				}
			}
		}
	}
	fclose(test);
	fclose(result);
	return 0;
}