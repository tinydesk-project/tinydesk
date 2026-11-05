/* Native account management. Credentials never enter shell history. */
#include "td_apps.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static td_window_t *s_win;
static td_widget_t *s_list,*s_name,*s_admin,*s_rights[7],*s_status;
static td_widget_t *s_old,*s_new,*s_confirm,*s_hint_box;
static char s_old_password[129],s_new_password[129],s_confirm_password[129];
static char s_hint[TD_HINT_MAX];   /* the hint box's text */
static td_user_info_t s_selected;
static bool s_creating;
static int s_last_index;
static const char *const s_labels[]={"Manage accounts","System settings","Network","Hardware / I/O","Shared files","Clock","Apps / Terminal"};
static const td_user_ops_t *ops(void){return td_sysinfo()->users;}
static bool manager(void){return !td_session_locked()&&td_session_role()>=TD_USER_ROLE_ADMIN&&td_session_has_right(TD_RIGHT_USERS);}
static bool editable(void){return manager()&&(s_creating||(s_selected.role!=TD_USER_ROLE_ROOT&&strcmp(s_selected.name,td_session_user())));}
static void wipe(void)
{
    volatile char *a=s_old_password,*b=s_new_password,*c=s_confirm_password;
    for(unsigned i=0;i<sizeof(s_old_password);i++)a[i]=b[i]=c[i]=0;
    if(s_old)s_old->value=s_old->scroll=0;
    if(s_new)s_new->value=s_new->scroll=0;
    if(s_confirm)s_confirm->value=s_confirm->scroll=0;
}
/* A hint shown at Sign in must not give the password away. */
static bool hint_reveals(const char *hint,const char *password)
{
    size_t n=strlen(password),m=strlen(hint);
    if(!n||!m)return false;
    if(n<3)return n==m&&!strcasecmp(hint,password);
    for(size_t i=0;i+n<=m;i++)if(!strncasecmp(hint+i,password,n))return true;
    return false;
}
static void set_hint_box(const char *text)
{
    if(!s_hint_box)return;
    snprintf(s_hint,sizeof(s_hint),"%s",text);s_hint_box->value=(int)strlen(s_hint);s_hint_box->scroll=0;
}
static void note(const char *message){td_widget_set_text(s_status,message);if(td_win_is_open(s_win))td_win_invalidate(s_win);}
static int visible_user(int index,td_user_info_t *out)
{
    if(!ops()||!ops()->list)return -1;
    for(int i=0;i<12;i++){td_user_info_t u;if(ops()->list(i,&u))break;if(!manager()&&strcmp(u.name,td_session_user()))continue;if(index--==0){*out=u;return 0;}}
    return -1;
}
static const char *item(td_widget_t *list,int index,int *fg,void *user)
{
    (void)list;(void)fg;(void)user;static char text[40];td_user_info_t u;
    if(visible_user(index,&u))return "";
    snprintf(text,sizeof(text),"%s",u.name);return text;
}
static void refresh_count(void)
{
    td_user_info_t u;int n=0;while(n<12&&!visible_user(n,&u))n++;td_list_set_count(s_list,n);
}
static void select_user(td_widget_t *list,void *user)
{
    (void)user;s_creating=false;s_last_index=list->value;wipe();
    if(visible_user(list->value,&s_selected)){note("No account selected.");return;}
    td_widget_set_text(s_name,s_selected.name);s_name->focusable=false;
    char hint[TD_HINT_MAX]="";if(ops()->hint_get)(void)ops()->hint_get(s_selected.name,hint,sizeof(hint));set_hint_box(hint);
    td_checkbox_set(s_admin,s_selected.role>=TD_USER_ROLE_ADMIN);
    for(int i=0;i<7;i++)td_checkbox_set(s_rights[i],s_selected.rights&(1u<<i));
    note(s_selected.role==TD_USER_ROLE_ROOT?"Root is protected. Its own password may be changed below.":manager()?"Change role / permissions, or set a new password.":"Your permissions are set by an administrator.");
}
static void role_changed(td_widget_t *w,void *user)
{
    (void)user;
    if(!editable()){td_checkbox_set(w,s_selected.role>=TD_USER_ROLE_ADMIN);note("These account permissions are read-only.");return;}
    if(!s_creating)return;
    uint32_t rights=td_checkbox_get(w)?TD_RIGHT_ALL&~TD_RIGHT_HARDWARE:TD_RIGHT_APPS;
    for(int i=0;i<7;i++)td_checkbox_set(s_rights[i],(rights&(1u<<i))&&td_session_has_right(1u<<i));
}
static void rights_changed(td_widget_t *w,void *user)
{
    (void)w;(void)user;if(editable())return;
    for(int i=0;i<7;i++)td_checkbox_set(s_rights[i],s_selected.rights&(1u<<i));
    note("These account permissions are read-only.");
}
static void new_account(td_widget_t *w,void *user)
{
    (void)w;(void)user;if(!manager()){note("An account administrator is required.");return;}
    s_creating=true;wipe();memset(&s_selected,0,sizeof(s_selected));td_widget_set_text(s_name,"");s_name->focusable=true;set_hint_box("");
    td_checkbox_set(s_admin,false);for(int i=0;i<7;i++)td_checkbox_set(s_rights[i],(1u<<i)==TD_RIGHT_APPS&&td_session_has_right(TD_RIGHT_APPS));
    note("Enter a name, new password and confirmation; then Create / Apply.");td_widget_focus(s_name);
}
static uint32_t requested_rights(void)
{
    uint32_t value=0;for(int i=0;i<7;i++)if(td_checkbox_get(s_rights[i]))value|=1u<<i;return value;
}
static void result(int rc)
{
    note(!rc?"Saved.":rc==-EACCES?"Not permitted for this account or these rights.":rc==-EEXIST?"An account with that name already exists.":rc==-ENOSPC?"The device account limit has been reached.":rc==-EINVAL?"Check the account name, role and password.":"Unable to save. Refresh the account and try again.");
}
static void apply(td_widget_t *w,void *user)
{
    (void)w;(void)user;if(!manager()){note("An account administrator is required.");return;}
    int role=td_checkbox_get(s_admin)?TD_USER_ROLE_ADMIN:TD_USER_ROLE_USER,rc=-1;
    if(s_creating){
        if(!s_new_password[0]||strcmp(s_new_password,s_confirm_password)){note("Enter matching new passwords.");return;}
        if(hint_reveals(s_hint,s_new_password)){note("The hint must not contain the password.");return;}
        char name[32];snprintf(name,sizeof(name),"%s",td_widget_text(s_name));
        if(ops()->create)rc=ops()->create(td_session_user(),name,s_new_password,role,requested_rights());
        if(!rc&&s_hint[0]&&ops()->hint_set)rc=ops()->hint_set(td_session_user(),name,s_hint);
        wipe();if(!rc){refresh_count();select_user(s_list,NULL);}result(rc);
    }else{
        if(ops()->access_set)rc=ops()->access_set(td_session_user(),s_selected.name,role,requested_rights());
        if(!rc)select_user(s_list,NULL);
        result(rc);
    }
}
static void password(td_widget_t *w,void *user)
{
    (void)w;(void)user;if(s_creating){apply(w,user);return;}
    char old_hint[TD_HINT_MAX]="";if(ops()->hint_get)(void)ops()->hint_get(s_selected.name,old_hint,sizeof(old_hint));
    bool hint_changed=ops()->hint_set&&strcmp(old_hint,s_hint);
    if(!s_new_password[0]&&!s_confirm_password[0]&&hint_changed){   /* the hint alone */
        int rc=ops()->hint_set(td_session_user(),s_selected.name,s_hint);if(!rc){note("Hint saved.");return;}result(rc);return;
    }
    if(!s_new_password[0]||strcmp(s_new_password,s_confirm_password)){note("Enter matching new passwords.");return;}
    if(hint_reveals(s_hint,s_new_password)){note("The hint must not contain the password.");return;}
    int rc=ops()->password_set?ops()->password_set(td_session_user(),s_selected.name,s_old_password,s_new_password):-1;
    if(!rc&&hint_changed)rc=ops()->hint_set(td_session_user(),s_selected.name,s_hint);
    wipe();result(rc);
}
static void remember(td_widget_t *w,void *user)
{
    (void)w;(void)user;if(!td_session_is_root()||s_creating||!ops()->boot_user_set){note("Only root can enable remembered login.");return;}
    int rc=ops()->boot_user_set(td_session_user(),s_selected.name);note(rc?"Unable to change bootuser.":"bootuser saved. A successful password login is required first.");
}
static void require_password(td_widget_t *w,void *user)
{
    (void)w;(void)user;if(!td_session_is_root()||!ops()->require_password){note("Only root can change login policy.");return;}result(ops()->require_password(td_session_user()));
}
static void tick(td_window_t *win)
{
    (void)win;if(!s_creating&&s_list->value!=s_last_index)select_user(s_list,NULL);
}
static void closed(td_window_t *win)
{
    (void)win;wipe();s_win=NULL;s_list=s_name=s_admin=s_status=s_old=s_new=s_confirm=s_hint_box=NULL;s_hint[0]=0;memset(&s_selected,0,sizeof(s_selected));
}
static void launch(void)
{
    if(td_session_locked())return;
    if(td_win_is_open(s_win)){td_win_focus(s_win);return;}
    if(!ops()||!ops()->list){td_msgbox("Users","This port does not provide account management.","OK",NULL,NULL);return;}
    td_window_desc_t d={.title="Users",.rect=td_rect(-1,-1,76,23),.min_w=72,.min_h=21,.flags=TD_WIN_DEFAULT,.on_close=closed,.on_tick=tick,.tick_ms=200};
    s_win=td_win_create(&d);if(!s_win)return;
    td_label(s_win,1,0,20,"ACCOUNTS");s_list=td_list(s_win,td_rect(1,2,20,14),item,select_user,NULL);
    td_label(s_win,25,0,40,"Account name");s_name=td_textbox(s_win,25,1,44,31,NULL,NULL);
    s_admin=td_checkbox(s_win,25,3,"Administrator",false,role_changed,NULL);
    for(int i=0;i<7;i++)s_rights[i]=td_checkbox(s_win,i<4?25:49,5+(i<4?i:i-4),s_labels[i],false,rights_changed,NULL);
    (void)td_button(s_win,25,9,"Create / Apply",apply,NULL);
    td_label(s_win,25,10,44,"Current password (when changing your own)");
    s_old=td_textbox(s_win,25,11,44,128,NULL,NULL);s_new=td_textbox(s_win,25,13,44,128,NULL,NULL);s_confirm=td_textbox(s_win,25,15,44,128,NULL,NULL);
    td_label(s_win,25,12,44,"New password");td_label(s_win,25,14,44,"Confirm new password");
    if(s_old){td_textbox_set_buffer(s_old,s_old_password,sizeof(s_old_password));s_old->secret=true;}
    if(s_new){td_textbox_set_buffer(s_new,s_new_password,sizeof(s_new_password));s_new->secret=true;}
    if(s_confirm){td_textbox_set_buffer(s_confirm,s_confirm_password,sizeof(s_confirm_password));s_confirm->secret=true;}
    (void)td_button(s_win,25,16,"Change password",password,NULL);
    (void)td_button(s_win,1,16,"New account",new_account,NULL);
    (void)td_button(s_win,1,17,"Remember login",remember,NULL);
    (void)td_button(s_win,25,17,"Require password at boot",require_password,NULL);
    if(ops()->hint_get){
        td_label(s_win,25,18,44,"Password hint (shown at Sign in; optional)");
        s_hint_box=td_textbox(s_win,25,19,44,TD_HINT_MAX-1,NULL,NULL);
        if(s_hint_box)td_textbox_set_buffer(s_hint_box,s_hint,sizeof(s_hint));
    }
    s_status=td_label(s_win,1,-1,0,"");
    if(s_win->incomplete){td_win_close(s_win);return;}
    refresh_count();select_user(s_list,NULL);
}
static const td_app_t s_app={"Users",launch,"U "};
void td_users_register(void){td_app_register(&s_app);}
