# Accounts, desktop login and kiosk startup

TinyDesk uses TinyDesk Shell's existing password database. Firmware updates keep
existing account names, salted password hashes, files and board settings. New
role/permission metadata is stored separately in NVS and tied to the account's
name and salt. A missing sidecar makes an existing non-root account a User with
Apps permission. An unreadable password database fails closed; it is not
replaced by factory credentials.

## Signing in

Choose an account and enter its existing password. CrowPanel's **Keyboard**
button provides touch entry, Shift/symbols, Space, Backspace and Clear. Passwords
stay masked. Five incorrect attempts introduce a ten-second delay. Desktop
shortcuts, other windows, clipboard paste targets and the kiosk remain behind
the authentication gate until the shell confirms the new session. Signing out
closes windows, clears the terminal and clipboard, and detaches Python GUI
objects from the old session.

An account's generation changes when its password is replaced or the account is
recreated. Old desktop/shell sessions lose rights and filesystem access; TinyDesk
returns to Sign in. Permission changes are read from the provider rather than
cached in a mutable shell variable. Long-running native/root code remains
trusted code; these are embedded application permissions, not an operating
system process sandbox.

## Root, Admin and User

Root retains full access. Admins and Users remain inside their own
`/home/NAME`; **Admin does not grant access to `/root` or another user's home**.
The same boundaries apply to shell paths, Python file paths and desktop file
operations. Optional Shared permission adds `/shared`. Device firmware updates,
raw executable Python/native code, raw memory and board configuration remain
root operations.

| Permission | Delegated operations |
| --- | --- |
| Manage accounts | Admin role plus this permission can create/manage accounts with a subset of its own rights; it cannot edit root or elevate itself |
| System settings | Mouse/keyboard device settings and shell reboot |
| Network | Device Wi-Fi/network policy changes and server controls; existing ownership checks on saved networks still apply |
| Hardware / I/O | Shell GPIO/hardware-test commands; root-only raw memory and native executable code stay protected |
| Shared files | Read/write `/shared`, while private homes stay private |
| Clock | Set device clock and automatic time |
| Apps / Terminal | Launch desktop Terminal and run MicroPython, including kiosk apps |

New Users default to Apps / Terminal. New Admins default to all listed rights
except Hardware. Root may customize either role. Changes cannot grant rights
the acting administrator does not have. The native **Users** app is available
from Start. Select **New account**, enter a name and matching new passwords,
choose role/rights and **Create / Apply**. Existing accounts use Apply for
permissions and **Change password** for credentials; changing your own password
requires its current password. Normal users can view their own permissions and
change their own password. Passwords never become shell command-line arguments.

Shell equivalents:

```text
useradd operator user
useradd supervisor admin
userrole operator
userrights operator +shared +network
userrole supervisor user
passwd
login operator
logout
```

`useradd` asks for the password interactively. `userrole NAME admin|user` resets
rights to that role's defaults intersected with the acting admin's rights;
use `userrights` afterwards for exceptions. `userdel NAME [-f]` retains its
interactive confirmation/password workflow and refuses deletion of root.

## Remembered desktop login

Password login is the default. Root alone controls remembered login with the
existing `bootuser` command, the root login checkbox, or Users policy buttons.

```text
bootuser root                 # select/enable a remembered desktop identity
bootuser operator
bootuser --require-password   # require a password again; also erase old proof
bootuser                     # show the console's selected boot user
```

Enabling a name alone cannot mint a remembered-login proof. That account must
successfully authenticate with its password. Authentication and proof creation
are serialized with password updates. The proof is invalid after a password
change or account replacement. Existing legacy `boot_user` values without this
proof do not bypass desktop login. A valid remembered login restores behind a
black gate and proceeds directly to the normal welcome/kiosk flow, without
briefly showing the Sign in window. A timeout returns to password login.

Root may put `bootuser root` or `bootuser --require-password` in
`/root/.tdshrc.tdsh`. Startup scripts run before ordinary console commands, so
that explicit root policy can override a prior GUI preference. Requiring a
password erases the previous proof; re-enabling needs another verified login.
This extends the shell's existing boot-user selection; it does not silently
replace the standalone shell's established local-console startup behavior.

## Kiosk apps

Authentication happens before `kiosk.app`. After root signs in, an existing
root-owned kiosk package runs with its existing artwork and configuration.
A non-root account cannot run a package under `/root`; install its app in its
own home, or `/shared` with Shared and Apps permissions. The app's custom
welcome page belongs to the app and appears after login. Signing out stops the
current kiosk session.

Keep CrowPanel UART0 reserved for the machine when `console.uart = -1` is set.
Its existing BOOT-after-RESET maintenance startup temporarily enables the
console and suspends kiosk launch; it does not bypass authentication. On the
next normal reboot the machine UART and kiosk configuration are restored.

## Validation

The host suite covers authentication failures, touch entry, hidden-window and
shortcut isolation, gate-window reuse, shell acknowledgement/timeout, late
acknowledgements, stale generations, private/shared paths, revocation, valid
remembered startup without a login frame, and remembered-startup timeout.
The production ESP account-store source is tested with real SHA-256 and an
in-memory NVS implementation for migration, denied escalation, write failures,
corrupt-store handling, atomic login and remembered-proof invalidation.
