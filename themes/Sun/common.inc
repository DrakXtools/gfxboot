% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Main part.
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% look if we've been started from dvd side 2
%
% ( ) ==> ( true|false )
%
/is_second_side {
  syslinux usernote 1 and 0 ne and
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Should we check for 64bit?
%
% ( ) ==> ( true|false )
%
/do_64bit_check {
  syslinux usernote 2 and 0 ne and
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Install source is 64bit?
%
% ( ) ==> ( true|false )
%
/64bit_source {
  syslinux usernote 4 and 0 ne and
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Are we a dvd?
%
% ( ) ==> ( true|false )
%
/is_dvd {
  syslinux usernote 8 and 0 ne and
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% input event handling
%
% ( key ) ==> ( input_buffer menu_entry action )
%
% key
%  bit 0-7	ascii
%  bit 8-15	scan code
%  bit 16-32	status bits (ctrl, shift...)
%
% action
%    0:		ok, stay in input loop
%    1:		switch to text mode
%  >=2:		start linux
%
/KeyEvent {
  % timeout

  dup 0 eq { boot.buf buildcmdline 2 return } if

  debug 4 ge {
    % print keycode somewhere
    -1 settransparentcolor
    black setcolor
    500 0 moveto dup print "  " print
  } if

  dup 0xff00 and 16 shl over 0xff and dup 0xe0 eq { pop 0 } if add /key exch def
  16 shr 0xffff and /keystat exch def

  key keyTab eq keyStatus statusShift and 0 ne and {
    /key keyShiftTab def
  } if

  key

  config.keymap { mapkey } if

  dup 0xffffff and dup { exch } if pop

  debug 4 ge {
    % print mapped key somewhere
    -1 settransparentcolor
    black setcolor
    500 20 moveto dup print "  " print
  } if

  % some special keys
  debug.input

  % put key through normal input queue
  window.input

  pop

  window.action actExit eq {
    /window.action actNothing def
    "" -1 1 return
  } if

  window.action actCloseInfo eq {
    /window.action actNothing def
    "" -1 3 return
  } if

  window.action actPassword eq {
    /window.action actNothing def
    password.dialog {
      password.dialog .ed.buffer.list get 0 get
    } { "" } ifelse
    -1 3 return
  } if

  window.action actStart eq {
    /window.action actNothing def
    boot.buf buildcmdline 2 return
  } if

  window.action actRedraw eq {
    /window.action actNothing def
    main.redraw
  } if

  window.action actRedrawPanel eq {
    /window.action actNothing def
    panel.show
  } if

  window.action actInstallOK eq {
    /window.action actNothing def
    install.ok
  } if

  window.action actInstallCancel eq {
    /window.action actNothing def
    install.cancel
  } if

  boot.buf menu.entry 0
} def


/bc.cmd 256 string def

% ( option_string ) ==> ( cmdline menu_entry )
%
% grub:
%   just return
%
% syslinux & lilo:
%   check if the commmand line starts with the current kernel name or
%   "linux"; if not, put the kernel name in front of the command line. (This
%   is to keep compatibility with the old scheme requiring the user to write
%   the kernel name explicitly.)
%
/buildcmdline {
  menu.entry 0 lt { -1 return } if
  menu.entry menu.texts length ge { -1 return } if

  /bc.opts exch def
  /bc.kernel menu.texts menu.entry get def

  grub {

    /bc.addkernel false def

  } {

    /bc.addkernel true def

    [ bc.kernel "linux" ] 0 over length 1 sub 1 exch {
      over exch get

      bc.opts over eq {
        /bc.addkernel false def
      } {
        bc.opts over strstr 1 eq {
           bc.opts over length get ' ' eq {
             /bc.addkernel false def
           } if
        } if
      } ifelse

      pop

      bc.addkernel not { exit } if

    } for

    pop

    % special case: option is identical to label
    bc.kernel "apic" eq { /bc.addkernel true def } if

  } ifelse

  bc.addkernel {
    alt.kernel "" ne { alt.kernel } { bc.kernel } ifelse
    "%s " bc.cmd sprintf
  } {
    bc.cmd 0 0 put
  } ifelse

  cmdline.hidden "" ne {
    cmdline.hidden "%s " bc.cmd dup length add sprintf
  } if

  syslinux {
    xmenu.video 0 get 0 eq {
      "textmode=1 " bc.cmd dup length add sprintf
    } if
    video.modes.list xmenu.video 0 get get 0 get dup {
      "vga=0x%x " bc.cmd dup length add sprintf
    } {
      pop
    } ifelse

    config.lang {
      config.lang "en" ne {
        config.lang "lang=%s " bc.cmd dup length add sprintf
      } if
    } if

  } if

  do_driverupdate {
    "dud=1 " bc.cmd dup length add sprintf
  } if

  % add splash only if an entry already exists
  bc.cmd "splash" bootopt.find dup .undef ne {
    % remove existing entry
    dup skipnonspaces skipspaces strcpy pop

    % append new entry
    xmenu.splash .xm_current get splash.options exch get
    "%s " bc.cmd dup length add sprintf
  } {
    pop
  } ifelse

  xmenu.profile {
    profile.options xmenu.profile .xm_current get get dup "" ne {
      "%s " bc.cmd dup length add sprintf
    } { pop } ifelse
  } if

  xmenu.install {
    install.option "" ne {
      install.option "%s " bc.cmd dup length add sprintf
    } if
  } if

  bc.opts "%s " bc.cmd dup length add sprintf

  bc.cmd dropspaces

  debug 3 ge {
    0 0 moveto black setcolor
    bc.cmd print "<<  (press ESC) " print trace
  } if

  bc.cmd menu.entry
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( menu_entries_array cmdline_args_array defaultentry ) == > ( )
/MenuInit {
  bsplash.done

  0 setcolor 0 0 moveto screen.size fillrect loadpalette
  init

  /menu.entry -1 def

  /menu.dentry exch def
  /menu.args exch def
  /menu.texts exch def

  window.main
  dup window.init
      window.show

%  fadein_logo

  is_second_side {

    notimeout dvd_popup

  } 

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Setup boot option input field.
%
% ( ) == > ( )
%
/bootoptions.init {
  window.current .color.fg get setcolor
  window.current .ed.font get setfont
  boot.ed edit.hidecursor
  menu.args menu.entry get
  menu.texts menu.entry get
  bootpromptmap
  dup
  boot.ed exch edit.init
  "" ne { boot.ed ' ' edit.input } if
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Split command line into hidden and normal part.
%
% ( cmdline_args menu_text ) == > ( new_cmdline_args )
%
% alt.kernel is set if a different kernel should be used (this feature should
% no longer be needed).
%
/bootpromptmap {
  /alt.kernel "" def

%  syslinux {
%    dup "apic"  eq { pop pop "apic" /alt.kernel "linux" def return } if
%  } if
  pop

  /cmdline exch def

  cmdline "showopts" getoption
  dup "" eq {
    cmdline.shown 0 0 put
    pop cmdline "%s" 256 cmdline.shown snprintf
    cmdline.hidden 0 0 put
  } {
    "showopts" length add skipspaces
    "%s" 256 cmdline.shown snprintf
    cmdline "%s" 256 cmdline.hidden snprintf
    cmdline.hidden "showopts" getoption 0 0 put
  } ifelse

  cmdline.shown dropspaces
  cmdline.hidden dropspaces

  cmdline.shown
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Skip leading non-spaces.
%
% ( string ) ==> ( string )
%
/skipnonspaces {
  { dup 0 get dup 0 ne exch ' ' ne and { 1 add } { exit } ifelse } loop
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Skip leading spaces.
%
% ( string ) ==> ( string )
%
/skipspaces {
  { dup 0 get ' ' eq { 1 add } { exit } ifelse } loop
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Drop spaces at string end.
% Modifies string!
%
% ( string ) ==> ( )
%
/dropspaces {
  dup length
  dup 0 eq {
    pop pop
  } {
    1 sub
    -1 0 {
      over over get ' ' eq { over exch 0 put } { pop exit } ifelse
    } for
    pop
  } ifelse
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Test if string[ofs-1]/string[ofs] is a word boundary.
%
% ( string ofs ) ==> ( true|false )
%
% boundary is either space/non-space or non-space/(space|'=')
%
/iswordboundary {
  dup 0 eq { pop pop true return } if

  add dup 1 sub 0 get exch 0 get

  over ' ' eq over ' ' gt and { pop pop true return } if
  over ' ' gt over dup ' ' eq exch dup '=' eq exch 0 eq or or and { pop pop true return } if

  pop pop false
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Get boot option.
%
% ( cmdline option_name ) ==> ( option_start )
%
/getoption {
  /go.name exch def
  /go.cmdline exch def
  /go.pos 0 def
  {
    go.cmdline go.pos add go.name strstr dup {
       1 sub /go.pos exch def

       go.cmdline go.pos iswordboundary
       go.cmdline go.pos go.name length add iswordboundary
       and {
         go.cmdline go.pos add exit
       } {
         /go.pos go.pos 1 add def
       } ifelse
    } {
      pop "" exit
    } ifelse
  } loop
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/redrawmenu {
  menu.visible.entries menu.texts length lt menu.scrollbar and {
    /menu.vsb.height1
      menu.shift
      menu.sb.height mul menu.texts length div
    def

    /menu.vsb.height3
      menu.texts length menu.visible.entries menu.shift add sub
      menu.sb.height mul menu.texts length div
    def

    lightgray setcolor
    menu.sb.x menu.sb.y menu.vsb.height1 add moveto
    menu.sb.width menu.sb.height menu.vsb.height1 menu.vsb.height3 add sub
    fillrect

    menu.vsb.height1 0 ne {
      menu.sb.x menu.sb.y moveto
      currentpoint menu.sb.width menu.vsb.height1 image
    } if

    menu.vsb.height3 0 ne {
      menu.sb.x menu.sb.y menu.sb.height menu.vsb.height3 sub add moveto
      currentpoint menu.sb.width menu.vsb.height3 image
    } if

  } if

  menu.text.normal setcolor

  /x menu.start.x def
  /y menu.start.y def

  0 1 menu.visible.entries 1 sub {
   x y moveto currentpoint menu.bar.width menu.bar.height image
   x menu.text.xofs add y menu.text.yofs add moveto
   menu.texts exch menu.shift add get menuitemmap
   currentfont exch font.large setfont show setfont
   /y y menu.item.height add def
  } for

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( entry status ) ==> ( )
%
% status:
%  false	not selected
%  true		selected
%
/MenuSelect {
  /menu.status exch def

  /menu.idx over def

  menu.shift sub
  menu.item.height mul menu.start.y add
  menu.start.x exch
  moveto

  currentpoint
  menu.status {
    menu.bar.color
    setcolor menu.bar.width menu.bar.height fillrect
  } {
    currentpoint menu.bar.width menu.bar.height image
  } ifelse
  moveto

  menu.text.xofs menu.text.yofs rmoveto
  menu.status {
    menu.text.select
  } {
    menu.text.normal
  } ifelse
  setcolor
  menu.texts menu.idx get menuitemmap
  currentfont exch font.large setfont show setfont

  menu.status {
    % init boot options
    keepbootoptions .undef eq { bootoptions.init } if

    % set help context
    "main" help.setcontext
    menu.texts menu.idx get
    dup help.findpage "" eq {
      pop
    } {
      help.setcontext
    } ifelse
  } if

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( text errorcode ) ==> ( )
% errorcode:
%   0	normal info
%   1	fatal error
%   2	missing kernel
%   3   disk change
%   4   disk change failed
%

/info.tmpmsg 127 string def

/InfoBoxInit {
  /info.type exch def
  /info.msg exch def

  window.dialog

  info.type 1 eq {
    dup .title.bg red put
    dup .title txt_error_title put
    dup .buttons
    [
      button.reboot button.default actCloseInfo button.setaction
    ] put
  } {
    dup .title txt_info_title put
    dup .buttons
    [
      button.ok button.default actCloseInfo button.setaction
%      button.cancel button.notdefault actCloseInfo button.setaction
    ] put
  } ifelse

  info.type 2 eq
  is_second_side and {
    txt_dvd_warning info.msg
    over length info.msg length 1 add add "%s%s" exch
    dup string dup /info.msg exch def snprintf
  } if

  syslinux info.type 3 eq and {
    dup .title txt_change_disk_title put
    0 getinfo 1 add txt_insert_disk info.tmpmsg sprintf
    /info.msg info.tmpmsg def
  } if

  syslinux info.type 4 eq and {
    dup .title txt_change_disk_title put
    1 getinfo 15 not and {
      0 getinfo 1 add
      txt_insert_disk3 info.tmpmsg sprintf
    } {
      0 getinfo 1 add 1 getinfo 1 add
      txt_insert_disk2 info.tmpmsg sprintf
    } ifelse
    /info.msg info.tmpmsg def
  } if

  dup .text info.msg put

  dup window.init
      window.show

} def



% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

% progress bar code


% Show percentage of progress bar.
%
% ( percentage ) ==> ( )
%
/progress.percent {
  0 max 100 min		% so people don't ask silly questions...
  "100%" strsize over neg progress.text.x add progress.text.y moveto
  window.current .color.bg get setcolor
  fillrect  

  "%3u%%" 8 dup string dup 5 1 roll snprintf

  dup strsize pop neg progress.text.x add progress.text.y moveto
  window.current .color.fg get setcolor
  dup show
  free

} def


% Show n-th progress bar symbol.
%
% ( n ) ==> ( )
%
/progress.sym.show {
  /progress.sym.current exch def

  progress.bar.x progress.bar.y moveto
  progress.sym.width progress.sym.current 1 sub mul 1 add 1 rmoveto
  progress.sym.width 2 sub
  progress.bar.height 2 sub
  loading_color setcolor
  fillrect

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( kernel_name ) ==> ( )
/ProgressInit {
  /progress.kname exch def

  boot.ed edit.hidecursor

  /dia window.dialog def

  dia .width.min 330 put
  dia .position 10 put

  dia .title txt_load_kernel_title put
  dia .text
    progress.kname "memtest" eq {
      txt_load_memtest
    } {
      txt_load_kernel
    } ifelse
  put

  dia window.init
  dia window.show

  % now add progress bar

  dia .x get dia .y get moveto
  dia .text.x get dia .text.y get 28 add rmoveto

  /progress.bar.height 19 def
  /progress.bar.width dia .width get 60 sub def

  /progress.sym.width 10 def
  /progress.bar.width
    progress.bar.width progress.sym.width div
    /progress.syms over def progress.sym.width mul
  def

  currentpoint over 1 sub over 2 sub moveto
  black white progress.bar.width 2 add progress.bar.height 4 add drawborder

  /progress.bar.y exch def
  /progress.bar.x exch def

  /progress.text.x progress.bar.x progress.bar.width 37 add add def
  /progress.text.y progress.bar.y progress.bar.height fontheight sub 2 div add def

  /progress.sym.current 0 def

  0 progress.percent

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( ) ==> ( )
/ProgressDone {
  window.done
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( max current ) ==> ( )
/ProgressUpdate {
  over over 100 mul exch 1 max div progress.percent

  progress.syms mul progress.syms 2 div add exch 1 max div

  0 max progress.syms min

  dup progress.sym.current gt {
    progress.sym.current 1 add over 1 exch {
      progress.sym.show
    } for
  } if
  pop

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( timeout time ) ==> ( )
/Timeout {
  % first time
  timeout.time .undef eq { timeout.init } if

  dup /timeout.time exch def

  over sub neg timeout.symbols mul exch div

  dup timeout.current eq { pop return } if

  /timeout.last timeout.current def
  /timeout.current exch def

  timeout.current timeout.symbols ge {
    timeout.x timeout.time.y moveto
    currentpoint timeout.dx timeout.symbols mul fontheight image
    0 1 timeout.symbols 1 sub {
%      timeout.clear { 2 } { 1 } ifelse drawtsymbol
    } for
  } {
    timeout.time.x timeout.time.y moveto
    currentpoint currentpoint 100 fontheight image
    moveto
    white setcolor
    timeout.time 10 mul 150 add 182 div "%ds" 64 timeout.buf snprintf timeout.buf show
    timeout.last 1 timeout.current {
      1 sub dup 0 ge {
%        1 drawtsymbol
      } if
    } for
  } ifelse
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% draw a timeout symbol
% ( index status ) ==> ()
%
% status: 0: init, 1: clear, 2: set
%
/drawtsymbol {
  /timeout.status exch def
  dup timeout.dx mul timeout.x add exch
  timeout.dy mul timeout.y add
  moveto currentpoint
  timeout.status 2 eq {
    currentpoint timeout.size image
  } {
    timeout.status 0 eq {
      pop pop
    } {
      0 462 timeout.size image
    } ifelse
  } ifelse
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( time ) ==> ( )
% /Timer { pop } def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( label correct_password ) ==> ( )
%
/PasswordInit {
  /password.key exch def pop

  /dia window.dialog def

  /password.dialog dia def

  dia .title txt_password_title put
  dia .text "" put

  % Must all be of same size!
  dia .ed.list 1 array put
  dia .ed.buffer.list [ 31 string ] put
  dia .ed.text.list [ txt_password ] put

  dia .ed.focus 0 put

  dia .ed.width 200 put

  dia .buttons
    [ button.ok button.default actPassword button.setaction ]
  put

  dia window.init
  dia window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( password ) ==> ( error )
%
% error:
%   true   password ok
%   false  wrong password
%
% ****** FIXME: test result seems to be unused
%
/PasswordDone {

  password.key eq
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( text ) == > ( new_text )
/menuitemmap {
  translate
  dup "memtest" eq over "memtest86" eq or { pop txt_memtest return } if
  syslinux {
    dup "linux"     eq { pop txt_install        return } if
    dup "failsafe"  eq { pop txt_safe_install   return } if
    dup "noacpi"    eq { pop txt_noacpi_install return } if
    dup "manual"    eq { pop txt_manual_install return } if
    dup "rescue"    eq { pop txt_rescue         return } if
    dup "hwcheck"   eq { pop "Hardware Check"   return } if
    dup "harddisk"  eq { pop txt_boot_harddisk  return } if
    dup "eval"      eq { pop "LiveEval"         return } if
  } {
    dup "linux"     eq { pop "Java Desktop System" return } if
    dup "Linux"     eq { pop "Java Desktop System" return } if
    dup "failsafe"  eq { pop "Java Desktop System - Safe Mode" return } if
    dup "windows"   eq { pop "Windows"      return } if
  } ifelse
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( color0 color1 width height ) ==> ( )
/drawborder {
  currentpoint /db.y0 exch def /db.x0 exch def

  /db.y1 exch 1 sub db.y0 add def
  /db.x1 exch 1 sub db.x0 add def
  /db.col1 exch def
  /db.col0 exch def

  db.x0 db.y1 moveto

  db.col0 setcolor
  db.x0 db.y0 lineto db.x1 db.y0 lineto

  db.col1 setcolor
  db.x1 db.y1 lineto db.x0 db.y1 lineto
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( color0 color1 color2 width height ) ==> ( )
% draw frame with shadow
% color0: upper left, color1: lower right, color2: shadow
/drawborder3 {
  currentpoint /db.y0 exch def /db.x0 exch def

  /db.y1 exch 1 sub db.y0 add def
  /db.x1 exch 1 sub db.x0 add def
  /db.col2 exch def
  /db.col1 exch def
  /db.col0 exch def

  db.x0 db.y1 moveto

  db.col0 setcolor
  db.x0 db.y0 lineto db.x1 db.y0 lineto

  db.col1 setcolor
  db.x1 db.y1 lineto db.x0 db.y1 lineto

  db.col2 -1 ne {
    db.col2 setcolor
    1 1 rmoveto
    db.x1 1 add db.y1 1 add lineto
    db.x1 1 add db.y0 1 add lineto
  } if
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( color0 color1 width height ) ==> ( )
/drawborder4 {
  3 index 4 1 roll rot dup 3 index 3 index
  currentpoint 6 2 roll
  1 1 rmoveto drawborder
  moveto drawborder
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% center text
% ( text width height ) ==> ( )
/centertext {
  3 -1 roll strsize
  4 2 roll
  4 1 roll exch 4 1 roll sub 2 div neg 3 1 roll sub 2 div neg
} def


/fadeout_logo {
  /cols 256 malloc def

  0 1 255 { cols exch 0 put } for

  10 1 150 {
    10 1 100 {
       over exch moveto getpixel
       cols exch 1 put
    } for
    pop
  } for
  cols 10 10 moveto getpixel 0 fade
} def

/fadein_logo {
  cols 10 10 moveto getpixel fadein
} def


% Allocate and define a new color.
%
% ( palette ) ==> ( color )
%
/newcolor {
  colorbits 8 le {
    newcolor.count .undef eq { /newcolor.count 0 def } if
    max_image_colors newcolor.count add
    dup rot setpalette
    /newcolor.count newcolor.count 1 add def
  } if
  def
} def


/init {
  0 0 moveto currentpoint clip.size image

  % set default language
  "lang" findfile dup {
    /tmp over length 1 add 2 max string def
    tmp exch {
      dup ' ' eq over '\n' eq or { pop pop exit } if
      over exch 0 exch put
      1 add
    } forall
    tmp 2 0 put % only short locale name
    tmp dup setlang pop setkeymap
  } {
    pop
    "en" dup setlang pop setkeymap
  } ifelse

  font.large setfont

  /menu.text.xofs      10 def
  /menu.text.yofs       2 def
  /menu.item.height    fontheight dup 3 div add def
  /menu.bar.height     fontheight menu.text.yofs dup add add def

  font.normal setfont

  /menu.text.normal    darkgray	         def
  /menu.text.select    black             def
  /boot.text.options   green             def
  /boot.text.normal    white             def
  /infobox.bg          yellow            def
  /infobox.text.normal black             def
  /frame.darkcolor     mediumgray        def
  /frame.lightcolor    lightgray         def
  /frame.shadowcolor   darkgray          def
  /menu.bar.color      lightgray         def

  /frame1.pos  { 140 142 } def
  /frame1.size { 340 178 } def
  /frame2.pos  { 150 328 } def
  /frame2.size { 370  22 } def
  /frame3.pos  { 150 306 } def
  /frame3.size { 120  23 } def
  /frame4.pos  { 490 142 } def
  /frame4.size {  50 178 } def

  /menu.start.x frame1.pos pop 10 add def
  /menu.start.y frame1.pos exch pop 12 add def

  /menu.bar.width frame1.size pop 20 sub def
  /menu.max.entries 8 def
  /menu.scrollbar false def

  /boot.buf.size 256 def
  /boot.buf boot.buf.size string def

  /ms.size     {  22  22 } def
  /ms.up       {   0 480 }  def
  /ms.down     {  23 480 }  def

  /update.pos { clip.size pop txt_dud_ready strsize pop sub 26 sub 360 } def

%  frame1.pos moveto frame_color dup dark_frame_color frame1.size drawborder3
%  frame1.pos moveto frame_color dark_frame_color frame1.size drawborder4


} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Some special debug & test keys.
%
% ( key_in ) ==> ( key_out )
%
/debug.input {
%  dup keyShiftF4 eq {
%    0 0 moveto
%    16 string dup dup "1234" ">%s<" 2 index sprintf show free
%    pop 0
%  } if

  dup keyF7 eq syslinux and {
    "kroete.data" findfile kroete.dir idle
    /kroete.dir kroete.dir 1 xor def
    pop 0
  } if

  dup keyF8 eq syslinux and {
    .undef 0 idle
    pop 0
  } if

  dup keyShiftF3 eq syslinux and debug 3 ge and {
    currentcolor black setcolor
    currentpoint 0 0 moveto
    "eject  " print bootdrive eject print
    moveto setcolor
    pop 0
  } if

  dup keyShiftF5 eq syslinux and debug 3 ge and {
    currentcolor black setcolor
    currentpoint 100 0 moveto
    bootdrive print
    moveto setcolor
    pop 0
  } if

  dup keyShiftF8 eq debug 3 ge and {
    currentcolor black setcolor currentpoint 300 0 moveto
    memsize print "/" print print "  " print
    moveto setcolor
    pop 0
  } if

  dup keyShiftF9 eq debug 3 ge and {
    pop 0
  } if

  dup keyShiftF10 eq {
    /debug debug 1 add def ""
    pop 0
  } if

  dup keyShiftF11 eq {
    currenttransparency 0x10 sub 0 max settransparency
    pop 0
  } if

  dup keyF11 eq {

    0 1 479 {
      0 1 639 {
        over moveto
        currentpoint 3 div 8 shl exch 3 div add setcolor
        putpixel
      } for
      pop
    } for

    pop 0
  } if

  dup keyShiftF12 eq {
    currenttransparency 0x10 add 0x100 min settransparency
    pop 0
  } if

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show exit popup.
%
% ( ) ==> ( )
%
/exit_popup {
  window.dialog

  dup .title txt_exit_title put
  dup .text txt_exit_dialog put
  dup .buttons [
    button.ok button.default actExit button.setaction
    button.cancel button.notdefault actNothing button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show help window.
%
% ( ) ==> ( )
%
/show_help {
  window.help
  
  dup window.init
  window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show dvd popup.
%
% ( ) ==> ( )
%
/dvd_popup {
  window.dialog

  dup .title txt_dvd_warning_title put
  dup .text txt_dvd_warning2 put
  dup .buttons [
%    button.eject    button.default    actEject actNoClose or button.setaction
    button.continue button.default actNothing button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Warn that we are about to install 32bit software on a 64bit system.
%
% ( ) ==> ( )
%
/32bit_popup {
  window.dialog

  dup .title "Cool computer, but..." put
  dup .text "You are about to install 32-bit software on a 64-bit computer." put
  dup .buttons [
    button.continue button.default actNothing button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Warn that we are about to install 64bit software on a 32bit system.
%
% ( ) ==> ( )
%
/64bit_popup {
  window.dialog

  dup .title "Cool software, but..." put
  dup .text "This is a 32-bit computer. You cannot use 64-bit software on it." put
  dup .buttons [
    button.reboot button.default actReboot actNoClose or button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show "power off" popup.
%
% ( ) ==> ( )
%
/power_off {
  window.dialog

  dup .title txt_power_off_title put
  dup .text txt_power_off put
  dup .buttons [
    button.ok     button.notdefault    actPowerOff actNoClose or button.setaction
    button.cancel button.default       actNothing                button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Initialize timeout indicator.
%
% ( ) ==> ( )
%
/timeout.init {
  /timeout.current    -1 def
  /timeout.x         260 def
  /timeout.y         358 def
  /timeout.dx          1 def
  /timeout.dy          0 def
  /timeout.symbols   320 def
  /timeout.clear    true def
  /timeout.size      { 1 18 } def
  /timeout.buf        64 string def

%  timeout.x timeout.y moveto
%  frame_color dup
%  timeout.dx timeout.symbols mul timeout.size exch pop
%  drawborder

  timeout.x timeout.y moveto
  white setcolor
  "booting in " show
  currentpoint /timeout.time.y exch def /timeout.time.x exch def
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Search for option in cmdline.
% Returns .undef if not found.
%
% ( cmdline option_name ) ==> ( option_start )
%
/bootopt.find {
  /bo.opt exch def
  /bo.cmdline exch def

  {
    bo.cmdline bo.opt strstr
    dup {
      dup 1 eq {
        true
      } {
        dup 2 sub bo.cmdline exch get ' ' eq
      } ifelse

      {
        bo.cmdline over bo.opt length add 1 sub get
        dup '=' eq
        over ' ' eq or
        exch 0 eq or
      } {
        false
      } ifelse

      bo.cmdline rot add exch

      {
        1 sub exit
      } {
        /bo.cmdline exch def
      } ifelse
    } {
      pop
      .undef exit
    } ifelse
  } loop

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Global variables.
%

/clip.size { 640 480 } def

640 480 8 findmode setmode not { false .end } if
% 800 600 8 findmode setmode not { false .end } if

"background.pcx" findfile setimage loadpalette
/max_image_colors image.colors def

% 7 dup 0xffffff setpalette setcolor

bsplash.show

% color & font definitions must be global

/black               0x000000          newcolor
/white               0xffffff          newcolor
/purple		     0x524c7a	       newcolor
/lightgray           0xc1c1c1          newcolor
/mediumgray	     0x8f8f8f	       newcolor
/medium6gray	     0x6e6e6e	       newcolor
/darkgray            0x4c4c4c          newcolor
/blue                0x072766          newcolor
/mediumblue          0x497782          newcolor
/lightblue           0x295d80          newcolor
/sunblue             0x072766          newcolor
/red                 0xc00000          newcolor
/somered             0xe6373a          newcolor
/green               0x009000          newcolor
/yellow              0xffff20          newcolor

%% include theme_colors.inc

/font.normal "16x16.font" findfile def
/font.large font.normal def

/cmdline.hidden 256 string def
/cmdline.shown 256 string def

/kroete.dir 0 def

/debug 0 def

