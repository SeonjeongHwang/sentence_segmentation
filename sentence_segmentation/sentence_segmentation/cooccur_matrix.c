#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 1715
#define TSIZE 1048576
#define SEED 1159241
#define WINDOW_S 15
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

typedef struct sent {
	char *wd;
	int index;
}SENTND;

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

void printinghash(HASHND ** ht) {
	HASHND *htmp;
	int index = 0;
	int total = 0;
	for (int i = 0; i < 1048576; i++) {
		htmp = ht[i];
		if (htmp == NULL) {
			continue;
		}
		for (;htmp != NULL; htmp = htmp->next) {
			//printf("(%s, %d)\n", htmp->wd, htmp->count);
			total += htmp->count;
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
	unsigned int indarr[MAX_WORD], w1_ind, w2_ind;
	int ind = 0;
	int flag;

	while (!feof(fin)) {
		int i, j, num;
		double weight, part_w, temp;
		int n = get_word(word, fin);
		//printf("%s\n", word);
		if (n) {
			for (i = 0; i < ind; i++) {
				w1_ind = indarr[i];
				flag = 0;
				for (j = i - WINDOW_S, weight = 0.7; j < i + WINDOW_S && j < ind; j++) {
					if (j < 0 || j >= WINDOW_S * 2 + 1) continue;
					w2_ind = indarr[j];
					if (w1_ind == -1 || w2_ind == -1) W[w1_ind][w2_ind] = -1;
					else {
						if (weight < 1) weight += 0.3 / WINDOW_S;
						else weight -= 0.3 / WINDOW_S;
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

int main(void)
{
	HASHND **hashtb = inithashtable();
	char word[100];

	//hashtable에 단어별 빈도수, W에서의 index 삽입
	FILE *fin = fopen("data2.txt", "r");
	while (!feof(fin)) {
		int n = get_word(word, fin);
		if (n) continue;
		hashinsert(hashtb, word);
	}
	total_num = Windex;
	fclose(fin);

	printf("%d", total_num);

	//printinghash(hashtb);

	/*int count = 0;
	fin = fopen("data.txt", "r");
	while (!feof(fin)) {
		if (count > 200) break;
		count++;
		int ch = fgetc(fin);
		printf("%c", ch);
	}
	fclose(fin);*/

	fin = fopen("data2.txt", "r");
	if (make_W(hashtb, fin)) {
		printf("error occured in making_W\n");
		return 0;
	}
	fclose(fin);

	FILE *fout = fopen("matrixW.txt", "w");
	printf("%d\n", total_num);
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

	/*
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			w = (int)W[i][j];
			if (j == 4) printf("%f", W[i][j] - (double)w);
			else printf("%f ", W[i][j] - (double)w);
		}
		printf("\n");
	}
	*/

	return 0;
}