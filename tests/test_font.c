#include "td_test.h"
#include "tinydesk/td_font.h"
#include "tinydesk/td_picture.h"
#include <stdint.h>

static uint8_t data[96];
static void put16(unsigned at,unsigned value){data[at]=(uint8_t)value;data[at+1]=(uint8_t)(value>>8);}
static void put32(unsigned at,unsigned value){put16(at,value);put16(at+2,value>>16);}
static size_t fixture(int bpp)
{
    memset(data,0,sizeof(data));memcpy(data,"TDFONT01",8);
    put16(8,32);data[10]=(uint8_t)bpp;put16(12,3);put16(14,2);put16(16,3);put16(18,8);
    put32(20,32);put32(24,80);put32(28,86);
    put32(32,32);put32(36,80);put16(44,2); /* space, empty bitmap */
    put32(48,63);put32(52,80);put16(56,1);put16(60,2);
    put32(64,0xb0);put32(68,83);put16(72,1);put16(76,2);
    data[80]=data[81]=data[82]=bpp==1?0x80:0xf0;
    data[83]=bpp==1?0x80:0x80;data[84]=0;data[85]=bpp==1?0x80:0xf0;
    return 86;
}
int main(void)
{
    td_font_t f;int w,h;size_t n=fixture(1);CHECK(td_font_open(&f,data,n));
    CHECK_EQ(f.height,3);CHECK_EQ(f.count,3);
    td_font_measure(&f,"??",0,false,&w,&h);CHECK_EQ(w,4);CHECK_EQ(h,3);
    td_font_measure(&f,"?\n?\n",0,false,&w,&h);CHECK_EQ(w,2);CHECK_EQ(h,9);
    td_font_measure(&f,"? ?",4,true,&w,&h);CHECK_EQ(w,2);CHECK_EQ(h,6);
    td_font_measure(&f,"\xc2\xb0\xe2\x82\xac",0,false,&w,&h);CHECK_EQ(w,4);CHECK_EQ(h,3);
    uint8_t out[20];memset(out,17,sizeof(out));
    td_font_draw(&f,"?",out+2,4,3,231,1,0,false);
    CHECK_EQ(out[2],17);CHECK_EQ(out[3],231);CHECK_EQ(out[7],231);CHECK_EQ(out[0],17);CHECK_EQ(out[19],17);
    for(size_t i=0;i<n;i++)CHECK(!td_font_open(&f,data,i));
    data[64]=63;CHECK(!td_font_open(&f,data,n)); /* duplicate glyph */
    n=fixture(4);CHECK(td_font_open(&f,data,n));
    memset(out,0,sizeof(out));td_font_draw(&f,"\xc2\xb0",out+2,1,3,231,0,0,false);
    CHECK(out[2]!=0&&out[2]!=231);CHECK_EQ(out[3],0);CHECK_EQ(out[4],231);CHECK_EQ(out[1],0);CHECK_EQ(out[5],0);
    uint16_t rgb[5];for(int i=0;i<5;i++)rgb[i]=td_rgb565(13,45,77);
    uint16_t original=rgb[0];td_font_draw_rgb(&f,"\xc2\xb0",rgb+1,1,3,231,0,0,false);
    CHECK(rgb[1]!=original&&rgb[1]!=0xffff);CHECK_EQ(rgb[2],original);CHECK_EQ(rgb[3],0xffff);CHECK_EQ(rgb[0],original);CHECK_EQ(rgb[4],original);
    data[48]='!';CHECK(!td_font_open(&f,data,n)); /* fallback required */
    fixture(1);put32(52,95);CHECK(!td_font_open(&f,data,n));
    fixture(1);data[11]=1;CHECK(!td_font_open(&f,data,n));
    return TD_TEST_RESULT();
}
