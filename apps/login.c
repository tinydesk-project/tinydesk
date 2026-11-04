/* Native authentication UI. The WM gate hides other windows and shortcuts. */
#include "td_apps.h"
#include <stdio.h>
#include <string.h>

static td_window_t *s_win;
static td_widget_t *s_accounts,*s_password,*s_remember,*s_note,*s_sign_in,*s_keyboard_button;
static char s_secret[129],s_name[32]="root";
static char s_message[96];
static int s_fails;
static uint32_t s_retry;
static int s_selected=-1;
static bool s_keyboard,s_shift;
static int s_key_held=-1;
static const char *const s_keys[]={"1234567890-=`","qwertyuiop[]\\","asdfghjkl;'","zxcvbnm,./"};
static const char *const s_shift_keys[]={"!@#$%^&*()_+~","QWERTYUIOP{}|","ASDFGHJKL:\"","ZXCVBNM<>?"};
static void login_layout(void);

static void wipe_secret(void)
{
    volatile char *p=s_secret;for(unsigned i=0;i<sizeof(s_secret);i++)p[i]=0;
    if(s_password&&s_password->used){s_password->value=0;s_password->scroll=0;}
}
static bool prevent_close(td_window_t *win){(void)win;return false;}
static void login_closed(td_window_t *win)
{
    (void)win;wipe_secret();s_message[0]=0;s_win=NULL;s_accounts=s_password=s_remember=s_note=s_sign_in=s_keyboard_button=NULL;s_keyboard=false;s_key_held=-1;
}
static const char *login_item(td_widget_t *list,int index,int *fg,void *user)
{
    (void)list;(void)user;(void)fg;static char out[64];const td_user_ops_t *ops=td_sysinfo()->users;
    td_user_info_t info;
    if(ops&&ops->list&&ops->list(index,&info)==0){snprintf(out,sizeof(out),"%-16.16s %s",info.name,info.role==TD_USER_ROLE_ROOT?"Root":info.role==TD_USER_ROLE_ADMIN?"Admin":"User");return out;}
    return index==0?"root":"";
}
static void chosen(td_widget_t *list,void *user)
{
    (void)user;const td_user_ops_t *ops=td_sysinfo()->users;td_user_info_t info;
    if(td_session_auth_pending())return;
    if(ops&&ops->list&&ops->list(list->value,&info)==0)snprintf(s_name,sizeof(s_name),"%s",info.name);
    else snprintf(s_name,sizeof(s_name),"root");
    wipe_secret();
    s_selected=list->value;
    if(s_remember){s_remember->visible=!strcmp(s_name,"root");td_checkbox_set(s_remember,ops&&ops->remember_allowed&&ops->remember_allowed(s_name));}
    td_widget_focus(s_password);
}
static void keyboard_toggle(td_widget_t *button,void *user)
{
    (void)button;(void)user;s_keyboard=!s_keyboard;s_key_held=-1;
    login_layout();td_widget_focus(s_password);td_win_invalidate(s_win);
}
static void login_layout(void)
{
    if(!s_accounts||!s_password||!s_remember||!s_sign_in||!s_note||!s_keyboard_button)return;
    const td_stats_t *st=td_stats();int x=(st->cols-42)/2,y=(st->rows-19)/2;if(x<1)x=1;if(y<0)y=0;
    s_accounts->visible=!s_keyboard;
    if(s_keyboard){
        s_password->rect=td_rect(3,4,st->cols-6,1);
        s_remember->rect.x=3;s_remember->rect.y=18;
        s_sign_in->rect.x=3;s_sign_in->rect.y=20;
        s_keyboard_button->rect.x=20;s_keyboard_button->rect.y=20;
        s_note->rect=td_rect(3,22,st->cols-6,1);
    }else{
        s_password->rect=td_rect(x,y+11,40,1);
        s_remember->rect.x=x;s_remember->rect.y=y+13;
        s_sign_in->rect.x=x;s_sign_in->rect.y=y+15;
        s_keyboard_button->rect.x=x+14;s_keyboard_button->rect.y=y+15;
        s_note->rect=td_rect(x,y+17,42,1);
    }
    td_widget_set_text(s_keyboard_button,s_keyboard?"Accounts":"Keyboard");
}
/* Touch keys live inside the authentication gate: no extra window, and no
 * shell command or password bytes are emitted to the terminal. */
static int touch_key(int x,int y)
{
    int left=(td_stats()->cols-65)/2;if(left<1)left=1;
    for(int row=0;row<4;row++)if(y>=7+row*2&&y<9+row*2&&x>=left){int col=(x-left)/5;if(col<(int)strlen(s_keys[row]))return row*16+col;}
    if(y>=15&&y<17){if(x>=left&&x<left+10)return 100;if(x>=left+12&&x<left+30)return 101;if(x>=left+32&&x<left+46)return 102;if(x>=left+48&&x<left+60)return 103;}
    return -1;
}
static bool login_event(td_window_t *win,const td_event_t *event)
{
    if(!s_keyboard||td_session_auth_pending()||event->type!=TD_EV_MOUSE||event->button!=TD_BUTTON_LEFT)return false;
    int key=touch_key(event->x,event->y);
    if(event->action==TD_MOUSE_PRESS){s_key_held=key;return key>=0;}
    if(event->action!=TD_MOUSE_RELEASE)return false;
    int held=s_key_held;s_key_held=-1;if(key<0||key!=held)return false;
    if(key==100)s_shift=!s_shift;
    else if(key==103)wipe_secret();
    else {td_event_t press={.type=TD_EV_KEY,.key=key==101?' ':key==102?TD_KEY_BACKSPACE:(uint32_t)(unsigned char)(s_shift?s_shift_keys:s_keys)[key/16][key%16]};td_widget_focus(s_password);(void)td_widgets_event(win,&press);}
    td_win_invalidate(win);return true;
}
void td_login_status(const char *message)
{
    snprintf(s_message,sizeof(s_message),"%s",message?message:"");
    if(s_note)td_widget_set_text(s_note,s_message);
    if(td_win_is_open(s_win))td_win_invalidate(s_win);
}
static void sign_in(td_widget_t *button,void *user)
{
    (void)button;(void)user;uint32_t now=td_millis();
    if(td_session_auth_pending()||(int32_t)(s_retry-now)>0)return;
    const td_sysinfo_t *si=td_sysinfo();const td_user_ops_t *ops=si->users;
    td_user_info_t selected_user;if(ops&&ops->list&&s_accounts&&ops->list(s_accounts->value,&selected_user)==0)snprintf(s_name,sizeof(s_name),"%s",selected_user.name);
    uint32_t generation=ops&&ops->login?ops->login(s_name,s_secret):0;
    bool authenticated=ops&&ops->login?generation!=0:si->authenticate&&si->authenticate(s_name,s_secret);
    if(!si->user_exists||!si->user_exists(s_name)||!authenticated){
        wipe_secret();s_fails++;s_retry=now+(s_fails>=5?10000:1000);td_login_status(s_fails>=5?"Too many attempts. Try again in 10 seconds.":"Incorrect user or password.");return;
    }
    wipe_secret();s_fails=0;s_retry=0;
    if(!strcmp(s_name,"root")&&ops){
        if(td_checkbox_get(s_remember)&&ops->boot_user_set)(void)ops->boot_user_set("root",s_name);
        else if(ops->require_password)(void)ops->require_password("root");
    }
    char name[sizeof(s_name)];snprintf(name,sizeof(name),"%s",s_name);
    td_login_status("Starting session...");td_session_accept_login(name,generation);
}
static void draw_login(td_window_t *win,int w,int h)
{
    (void)win;int x=(w-42)/2;if(x<1)x=1;int y=(h-19)/2;if(y<0)y=0;
    const td_theme_t *t=td_theme();td_fill(td_rect(0,0,w,h),' ',t->win_fg,t->win_bg);
    if(s_keyboard){
        td_text(3,1,"TINYDESK  /  SIGN IN",t->accent,t->win_bg,TD_BOLD);
        char label[64];snprintf(label,sizeof(label),"Password for %s",s_name);td_text(3,3,label,t->win_fg,t->win_bg,0);
        int left=(w-65)/2;if(left<1)left=1;
        for(int row=0;row<4;row++)for(int col=0;s_keys[row][col];col++){
            int xx=left+col*5,yy=7+row*2;td_fill(td_rect(xx,yy,4,1),' ',t->win_fg,t->title_bg);
            char text[2]={(s_shift?s_shift_keys:s_keys)[row][col],0};td_text(xx+1,yy,text,t->win_fg,t->title_bg,0);
        }
        td_text(left,15,s_shift?"[ SHIFT ]":"[ Shift ]",t->accent,t->win_bg,0);td_text(left+12,15,"[     Space     ]",t->win_fg,t->win_bg,0);
        td_text(left+32,15,"[ Backspace ]",t->win_fg,t->win_bg,0);td_text(left+48,15,"[ Clear ]",t->win_fg,t->win_bg,0);return;
    }
    td_text(x,y,"TINYDESK",t->accent,t->win_bg,TD_BOLD);
    td_text(x,y+2,"Choose an account and enter its password.",t->win_fg,t->win_bg,0);
    td_text(x,y+10,"Password",t->win_fg,t->win_bg,0);
    td_text(x,y+18,"Root controls remembered login with bootuser.",244,t->win_bg,0);
}
static void login_tick(td_window_t *win)
{
    (void)win;if(s_accounts&&s_accounts->value!=s_selected&&!td_session_auth_pending())chosen(s_accounts,NULL);
}
void td_login_open(void)
{
    if(td_win_is_open(s_win)){td_wm_set_input_gate(true,s_win);return;}
    td_wm_set_input_gate(true,NULL);
    const td_stats_t *st=td_stats();int x=(st->cols-42)/2,y=(st->rows-19)/2;if(x<1)x=1;if(y<0)y=0;
    td_window_desc_t d={.title="Sign in",.rect=td_rect(0,0,st->cols,st->rows),.flags=TD_WIN_FULLSCREEN|TD_WIN_MODAL,.on_draw=draw_login,.on_event=login_event,.on_close=login_closed,.on_close_request=prevent_close,.on_tick=login_tick,.tick_ms=200};
    s_win=td_win_create(&d);if(!s_win)return;
    s_accounts=td_list(s_win,td_rect(x,y+4,40,5),login_item,chosen,NULL);
    int count=0;td_user_info_t info;const td_user_ops_t *ops=td_sysinfo()->users;
    if(ops&&ops->list)while(count<12&&ops->list(count,&info)==0)count++;
    if(s_accounts)td_list_set_count(s_accounts,count?count:1);
    s_password=td_textbox(s_win,x,y+11,40,128,sign_in,NULL);
    if(s_password){td_textbox_set_buffer(s_password,s_secret,sizeof(s_secret));s_password->secret=true;}
    s_remember=td_checkbox(s_win,x,y+13,"Remember root on this device",false,NULL,NULL);
    s_note=td_label(s_win,x,y+17,42,s_message);
    s_sign_in=td_button(s_win,x,y+15,"Sign in",sign_in,NULL);
    s_keyboard_button=td_button(s_win,x+14,y+15,"Keyboard",keyboard_toggle,NULL);
    if(s_win->incomplete){td_win_close(s_win);return;}
    if(s_accounts)chosen(s_accounts,NULL);
    login_layout();
    td_wm_set_input_gate(true,s_win);
}
