/*
  Very Tiny Language

  T. Nakagawa

  2004/05/23
  2004/06/26
*/


#define MEMSIZE (1024 * 32)
#define READB(adrs) (*(Lct + (adrs) % MEMSIZE))
#define WRITEB(adrs, data) ((*(Lct + (adrs) % MEMSIZE) = (data)))
#define READW(adrs) ((unsigned short)(*(Lct + (adrs) % MEMSIZE) + *(Lct + ((adrs) + 1) % MEMSIZE) * 256))
#define WRITEW(adrs, data) (*(Lct + (adrs) % MEMSIZE) = (data) % 256, *(Lct + ((adrs) + 1) % MEMSIZE) = (data) / 256)


#define Nbf 0x82
#define Lbf 0x88
#define Svp ((2 + '!') * 2)
#define Pcc ((2 + '#') * 2)
#define Rmd ((2 + '%') * 2)
#define Bnd ((2 + '&') * 2)
#define Rnd ((2 + '\'') * 2)
#define Lmt ((2 + '*') * 2)
#define Obj 0x108

unsigned char Lct[MEMSIZE];

void initl(void) {
  WRITEW(Lmt, MEMSIZE);	/* RAM End */
  WRITEW(Bnd, Obj);	/* Program End */
  return;
}

unsigned char getchr(void) {
  int c;
  c = getch();
  if (c == '\x03') exit(0);	/* CTRL+C */
  if (c < 0) c = 0;
  putch(c);
  return (unsigned char)c;
}

void putchr(unsigned char c) {
  putch(c);
  return;
}

static int fndln(unsigned short *ptr);
static unsigned short nxtln(unsigned short ptr);
static void getln(unsigned short lbf);
static int getnm(unsigned short *ptr, unsigned short *n);
static int num(unsigned short ptr);
static void ordr(unsigned short ptr);
static void expr(unsigned short *ptr, unsigned short *val);
static void factr(unsigned short *ptr, unsigned short *val);
static void term(unsigned short *ptr, unsigned short *val);
static void getvr(unsigned short *ptr, unsigned char *c, unsigned short *adr);
static void putl(unsigned short *ptr, unsigned char d);
static void crlf(void);
static void putnm(unsigned short x);
static void putstr(char *str);


int main(void) {
  unsigned short ptr;
  unsigned short n;

  initl();
  putstr("VTL-C\r\n");

  for (; ; ) {
    ptr = Lbf + 2;
    getln(ptr);
    if (!getnm(&ptr, &n)) {
      unsigned short line;

      line = Lbf;
      WRITEW(line, 0);
      WRITEW(Pcc, 0);
      for (; ; ) {
	ordr(ptr);
	if (READW(Pcc) == 0 || READW(Pcc) == READW(line)) {
	  /* no branch */
	  if (line == Lbf) break;	/* �_�C���N�g���[�h�̏ꍇ */
	  line = nxtln(line);
	  if (line == READW(Bnd)) break;	/* �ŏI�s�܂Ŏ��s�����ꍇ */
	} else {
	  /* branch */
	  WRITEW(Svp, READW(line) + 1);
	  if (fndln(&line)) break;
	}
	WRITEW(Pcc, READW(line));
	ptr = line + 2 + 1;
      }
    } else {
      if (n == 0) {
	/* LIST */
	for (ptr = Obj; ptr != READW(Bnd); ) {
	  putnm(READW(ptr));
	  ptr += 2;
	  putl(&ptr, '\0');
	  crlf();
	}
      } else {
	unsigned short cur, src, dst, tmp;
	unsigned short m;

	/* DELETE */
	WRITEW(Pcc, n);
	if (!fndln(&cur) && READW(cur) == n) {
	  src = nxtln(cur);
	  for (dst = cur; src != READW(Bnd); WRITEB(dst++, READB(src++))) ;
	  WRITEW(Bnd, dst);
	}
	/* INSRT */
	if (READB(ptr) == '\0') continue;
	for (m = 3, tmp = ptr; READB(tmp) != '\0'; m++, tmp++) ;
	if (READW(Bnd) + m < READW(Lmt)) {
	  src = READW(Bnd);
	  WRITEW(Bnd, READW(Bnd) + m);
	  for (dst = READW(Bnd); src != cur; WRITEB(--dst, READB(--src))) ;
	  WRITEW(src, n);
	  src += 2;
	  while (WRITEB(src++, READB(ptr++)) != '\0') ;
	  continue;
	}
      }
    }
    putstr("\r\nOK");
  }

  return 0;
}


/*
  �s�ԍ���*Pcc�ȏ�̍ŏ��̍s��T���āC
  ������΂��̐擪�Ԓn��*ptr�ɓ����0��Ԃ��C������Ȃ����1��Ԃ��D
*/
static int fndln(unsigned short *ptr) {
  for (*ptr = Obj; *ptr != READW(Bnd); *ptr = nxtln(*ptr)) {
    if (READW(*ptr) >= READW(Pcc)) return 0;
  }
  return 1;
}


/*
  *ptr�̎��̍s�̐擪�������Ă��̈ʒu��Ԃ�
*/
static unsigned short nxtln(unsigned short ptr) {
  for (ptr += 2; READB(ptr++) != '\0'; ) ;
  return ptr;
}


/*
  ��s�ǂݍ����lbf����n�܂�A�h���X�Ɋi�[����
*/
static void getln(unsigned short lbf) {
  int p;
  unsigned char c;

  for (p = 0; ; ) {
    c = getchr();
    if (c == '\b') {
      if (p > 0) p--;
    } else if (c == '\r') {
      WRITEB(lbf + p, '\0');
      putchr('\n');
      return;
    } else if (c == 0x15 || p + 1 == 74) {
      crlf();
      p = 0;
    } else if (c <= 0x1f) {
    } else {
      WRITEB(lbf + p++, c);
    }
  }
}


/*
  **ptr�������łȂ����0��Ԃ�
  �����̏ꍇ�́C���̐������o�C�i���ɕϊ�����*n�ɓ���C
  *ptr�𐔎��̎��̃A�h���X�ɐi�܂��āC1��Ԃ�
*/
static int getnm(unsigned short *ptr, unsigned short *n) {
  if (!num(*ptr)) return 0;
  *n = 0;
  do {
    *n *= 10;
    *n += READB((*ptr)++) - '0';
  } while (num(*ptr));
  return 1;
}


/*
  *ptr�������Ȃ�1�C����ȊO�Ȃ�0��Ԃ�
*/
static int num(unsigned short ptr) {
  return ('0' <= READB(ptr) && READB(ptr) <= '9');
}


/*
  ptr����n�܂镶�����s����
*/
static void ordr(unsigned short ptr) {
  unsigned char c;
  unsigned short adr;

  getvr(&ptr, &c, &adr);	/* ���Ӓl�̃A�h���X�����߂� */
  ptr++;	/* �����'='��ǂݔ�΂� */

  if (READB(ptr) == '"') {
    ptr++;
    putl(&ptr, '"');
    if (READB(ptr) != ';') crlf();
  } else {
    unsigned short val;

    expr(&ptr, &val);	/* �E�Ӓl���v�Z���� */

    if (c == '$') {
      putchr(val & 0xff);
    } else if ((c -= '?') == 0) {
      putnm(val);
    } else {
      unsigned short r;

      WRITEW(adr, val);

      r = READW(Rnd);
      r += val;
      r = (r << 8) + (r >> 8);
      WRITEW(Rnd, r);
    }
  }
  return;
}


/*
  *ptr�Ŏn�܂�l���v�Z���C*val�ɓ����
*/
static void expr(unsigned short *ptr, unsigned short *val) {
  unsigned char c;

  factr(ptr, val);
  while ((c = READB(*ptr)) != '\0' && c != ')') {
    term(ptr, val);
  }
  (*ptr)++;
  return;
}


/*
  *ptr����n�܂�l�̗v�f��ǂ�*val�ɓ����
*/
static void factr(unsigned short *ptr, unsigned short *val) {
  unsigned char c;

  if (READB(*ptr) == '\0') {
    *val = 0;
    return;
  }

  if (getnm(ptr, val)) return;

  c = READB((*ptr)++);
  if (c == '?') {
    unsigned short tmp;

    tmp = Lbf;
    getln(tmp);
    expr(&tmp, val);
  } else if (c == '$') {
    *val = getchr();;
  } else if (c == '(') {
    expr(ptr, val);
  } else {
    unsigned short adr;

    (*ptr)--;
    getvr(ptr, &c, &adr);
    *val = READW(adr);	/* �ϐ����z��̒l�𓾂� */
  }
  return;
}


/*
  *ptr����n�܂鉉�Z�q�ƒl�̗v�f��ǂ݉��Z���ʂ�*val�ɓ����
*/
static void term(unsigned short *ptr, unsigned short *val) {
  unsigned char c;
  unsigned short val2;

  c = READB((*ptr)++);
  factr(ptr, &val2);
  if (c == '*') {
    *val *= val2;
  } else if (c == '+') {
    *val += val2;
  } else if (c == '-') {
    *val -= val2;
  } else if (c == '/') {
    WRITEW(Rmd, *val % val2);
    *val /= val2;
  } else if (c == '=') {
    *val = (*val == val2);
  } else if (c == '>') {
    *val = (*val >= val2);
  } else {
    *val = (*val < val2);
  }
  return;
}


/*
  *ptr����n�܂�ϐ��E�z��̕�����*c�ɓ���ăA�h���X��*adr�Ԃ�
*/
static void getvr(unsigned short *ptr, unsigned char *c, unsigned short *adr) {
  unsigned short val;

  *c = READB((*ptr)++);
  if (*c == ':') {
    expr(ptr, &val);
    *adr = READW(Bnd) + val * 2;
  } else {
    *adr = ((*c & 0x3f) + 2) * 2;
  }
  return;
}


/*
  ����d�ŏI�镶����ptr��\������
*/
static void putl(unsigned short *ptr, unsigned char d) {
  while (READB(*ptr) != d) putchr(READB((*ptr)++));
  (*ptr)++;
  return;
}


/*
  ���s����
*/
static void crlf(void) {
  putchr('\r');
  putchr('\n');
  return;
}


/*
  ���lx��\������
*/
static void putnm(unsigned short x) {
  unsigned short ptr;
  unsigned char y;

  ptr = Nbf + 5;
  WRITEB(ptr, '\0');
  do {
    y = x % 10;
    x /= 10;
    WRITEB(--ptr, y + '0');
  } while (x != 0);
  putl(&ptr, '\0');
  return;
}


/*
  ������str��\�����ĉ��s����
*/
static void putstr(char *str) {
  while (*str != '\0') putchr(*(str++));
  crlf();
  return;
}