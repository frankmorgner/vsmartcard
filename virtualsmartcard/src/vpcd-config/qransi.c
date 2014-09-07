/* Derived from https://github.com/bengl/qransi by Bryan English */

#include <stdio.h>
#include <stdlib.h>
#include <qrencode.h>

#define WHITE "\033[47m  \033[0m"
#define BLACK "\033[40m  \033[0m"

void pixels(char* color, int size)
{
  int i;
  for(i=0; i<size; i++)
    printf("%s", color);
}

void whiteRow(int padding, int width)
{
  int i;
  for(i=0; i<padding; i++) {
    pixels(WHITE, (width+(padding*2)));
    printf("\n");
  }
}

int qransi (const char *encodable)
{
  int padding = 2;
  QRecLevel level = QR_ECLEVEL_L;
  int x, y, width;
  unsigned char *data;
  QRcode * code = QRcode_encodeString(encodable, 0, level, QR_MODE_8, 1);
  if (!code)
    return 1;
  width = code->width;
  data = code->data;
  whiteRow(padding, width);
  for (y=0; y<width; y++) {
    pixels(WHITE, padding);
    for (x=0; x<width; x++, data++)
      pixels(*data&1?BLACK:WHITE, 1); 
    pixels(WHITE, padding);
    printf("\n");
  }
  whiteRow(padding, width);
  QRcode_free(code);
  return 0;
}
