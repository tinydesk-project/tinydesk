/* Authentication, shell acknowledgement and filesystem boundaries. */
#include "td_test.h"
#include "td_apps.h"
static uint32_t now=1000,rights=TD_RIGHT_APPS;
static char shell[32]="root",requested[32];
static int verified;
static uint32_t alice_generation=2,admin_generation=3;
static int read_byte(void *p){(void)p;return -1;}
static int write_bytes(void *p,const uint8_t *b,int n){(void)p;(void)b;return n;}
static uint32_t millis(void *p){(void)p;return now;}
static void sleep_ms(void *p,uint32_t ms){(void)p;now+=ms;}
static bool exists(const char *u){return !strcmp(u,"root")||!strcmp(u,"alice")||!strcmp(u,"admin");}
static bool authenticate(const char *u,const char *pw){return exists(u)&&!strcmp(pw,"test-password");}
static int role(const char *u){return !strcmp(u,"admin")?TD_USER_ROLE_ADMIN:TD_USER_ROLE_USER;}
static uint32_t user_rights(const char *u){(void)u;return rights;}
static int list(int index,td_user_info_t *out){const char *names[]={"root","alice","admin"};if(index<0||index>2)return -1;snprintf(out->name,sizeof(out->name),"%s",names[index]);out->role=index==0?TD_USER_ROLE_ROOT:role(names[index]);out->rights=user_rights(names[index]);return 0;}
static uint32_t generation(const char *u){return !strcmp(u,"root")?1:!strcmp(u,"alice")?alice_generation:!strcmp(u,"admin")?admin_generation:0;}
static uint32_t login_user(const char *u,const char *pw){if(!authenticate(u,pw))return 0;verified++;return generation(u);}
static const char *backend_user(void *p){(void)p;return shell;}
static void set_user(void *p,const char *u){(void)p;snprintf(requested,sizeof(requested),"%s",u);shell[0]=0;}
static void settle(void){for(int i=0;i<4;i++){now+=100;td_step();}}
static td_widget_t *widget(td_window_t *w,const char *text){for(td_widget_t *p=w->widgets;p;p=p->next)if(!strcmp(td_widget_text(p),text))return p;return NULL;}
int main(void)
{
    static const td_hal_t hal={read_byte,write_bytes,millis,sleep_ms,NULL};
    static const td_user_ops_t users={.list=list,.role=role,.rights=user_rights,.generation=generation,.login=login_user};
    static const td_fs_ops_t fs={.root="/fs"};
    static const td_sysinfo_t info={.user_exists=exists,.authenticate=authenticate,.users=&users,.fs=&fs,.kiosk_app="/root/private.tdapp"};
    static const td_term_backend_t backend={.user=backend_user,.set_user=set_user};
    td_set_sysinfo(&info);td_init(&hal);td_session_init();td_terminal_set_backend(&backend);
    CHECK(td_session_locked());CHECK(!td_session_is_root());CHECK(!td_session_has_right(TD_RIGHT_APPS));
    td_window_t *login=td_win_focused();CHECK(login!=NULL);
    td_widget_t *sign_in=widget(login,"Sign in");CHECK(sign_in!=NULL);
    td_widget_t *password=NULL;for(td_widget_t *w=login->widgets;w;w=w->next)if(w->secret)password=w;
    CHECK(password!=NULL);td_widget_set_text(password,"wrong");
    td_widget_t *keyboard=widget(login,"Keyboard");CHECK(keyboard!=NULL);keyboard->on_activate(keyboard,keyboard->user);
    td_widget_set_text(password,"");
    /* The shared keys (apps/osk.c) at 80 x 25: units of 6 columns from x 4,
     * rows of 3 lines from y 5. "1" (6,6), Shift (68,12), Clear (56,18). */
    td_event_t touch={.type=TD_EV_MOUSE,.button=TD_BUTTON_LEFT,.x=6,.y=6,.action=TD_MOUSE_PRESS};td_wm_dispatch(&touch);touch.action=TD_MOUSE_RELEASE;td_wm_dispatch(&touch);
    CHECK(!strcmp(td_widget_text(password),"1"));
    touch.x=68;touch.y=12;touch.action=TD_MOUSE_PRESS;td_wm_dispatch(&touch);touch.action=TD_MOUSE_RELEASE;td_wm_dispatch(&touch);
    touch.x=6;touch.y=6;touch.action=TD_MOUSE_PRESS;td_wm_dispatch(&touch);touch.action=TD_MOUSE_RELEASE;td_wm_dispatch(&touch);
    CHECK(!strcmp(td_widget_text(password),"1!"));
    touch.x=56;touch.y=18;touch.action=TD_MOUSE_PRESS;td_wm_dispatch(&touch);touch.action=TD_MOUSE_RELEASE;td_wm_dispatch(&touch);CHECK(!td_widget_text(password)[0]);
    keyboard->on_activate(keyboard,keyboard->user);td_widget_set_text(password,"wrong");
    sign_in->on_activate(sign_in,sign_in->user);CHECK(td_session_locked());CHECK_EQ(verified,0);CHECK(!td_widget_text(password)[0]);
    now+=1500;td_widget_set_text(password,"test-password");sign_in->on_activate(sign_in,sign_in->user);
    CHECK_EQ(verified,1);settle();CHECK(!td_session_locked());CHECK(td_session_is_root());CHECK(!strcmp(td_session_home(),"/fs/root"));
    td_session_lock();CHECK(td_session_locked());CHECK(!td_session_path_allowed("/fs/root/private",TD_FS_READ));
    td_session_accept_authenticated("alice");settle();CHECK(td_session_locked());CHECK(!strcmp(requested,"alice"));
    snprintf(shell,sizeof(shell),"alice");settle();CHECK(!td_session_locked());CHECK(!strcmp(td_session_home(),"/fs/home/alice"));
    CHECK(td_session_path_allowed("/fs/home/alice/data",TD_FS_READ));CHECK(!td_session_path_allowed("/fs/root/data",TD_FS_READ));
    CHECK(!td_session_path_allowed("/fs/home/alice/../admin/data",TD_FS_READ));CHECK(!td_session_path_allowed("/fs/home/alice\\..\\root",TD_FS_READ));
    CHECK(!td_session_path_allowed("/fs/home/alice",TD_FS_REMOVE));CHECK(!td_session_path_allowed("/fs/shared/data",TD_FS_READ));
    CHECK(!td_session_path_allowed("/fs/home/alice/",TD_FS_REMOVE));CHECK(!td_session_path_allowed("/fs/home/alice/.",TD_FS_REMOVE));
    rights|=TD_RIGHT_SHARED;CHECK(td_session_path_allowed("/fs/shared/data",TD_FS_WRITE));CHECK(!td_session_path_allowed("/fs/shared",TD_FS_REMOVE));
    CHECK(!td_session_path_allowed("/fs/shared/",TD_FS_REMOVE));CHECK(!td_session_path_allowed("/fs/shared/.",TD_FS_REMOVE));
    CHECK(!td_session_path_allowed("/fs/home/admin/data",TD_FS_READ));rights=TD_RIGHT_APPS;CHECK(!td_session_path_allowed("/fs/shared/data",TD_FS_READ));
    td_session_lock();td_session_accept_authenticated("admin");settle();now+=11000;settle();CHECK(td_session_locked());CHECK(!td_session_auth_pending());
    snprintf(shell,sizeof(shell),"admin");settle();CHECK(td_session_locked()); /* late acknowledgement cannot unlock a timed-out login */
    td_session_accept_authenticated("admin");settle();CHECK(!td_session_locked());CHECK_EQ(td_session_role(),TD_USER_ROLE_ADMIN);
    CHECK(!td_session_path_allowed("/fs/root/secrets",TD_FS_READ));
    admin_generation++;CHECK(td_session_locked());CHECK(!td_session_has_right(TD_RIGHT_APPS));settle();CHECK(td_session_locked());
    td_session_accept_login("alice",alice_generation);alice_generation++;snprintf(shell,sizeof(shell),"alice");settle();CHECK(td_session_locked());CHECK(!td_session_auth_pending());
    return TD_TEST_RESULT();
}
