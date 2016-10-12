% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Video mode selection dialog.
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Some global vars.
%
% video mode array fields
/.vm_mode    0 def
/.vm_flags   1 def
/.vm_options 2 def
/.vm_label   3 def
/.vm_width   4 def
/.vm_height  5 def

% .vm_flags:
% 
% bit 0: is vbe mode
%     1: supported display res
%

% buffer for translated menu string
%/video.submenu.smode.buf 64 string def
%/video.submenu.smode {
%  xmenu.smode .xm_title get exec "\x09\x09%s" video.submenu.smode.buf sprintf
%  video.submenu.smode.buf
%} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Create sort key for video modes.
%
% ( vm_index )  ==> ( sort_index )
%
/vmsortindex {
  video.modes.list exch get
  dup
  .vm_width get 16 shl
  exch .vm_height get add
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Swap video mode entries.
% (Helper for video mode sorting.)
%
% ( vm_index_1 vm_index_2 )  ==> ( )
%
/vmsortexch {
  over video.modes.list exch get
  over video.modes.list exch get
  video.modes.list
  5 -1 roll rot put
  video.modes.list 3 1 roll put
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Build video mode list.
%
% ( ) ==> ( )
%
/video.init {
  /xmenu.video .xm_size array def

  /xmenu xmenu.video def

  "ddc" findfile test1
  /video.res [
    displaysizes

    % add anything that crosses your mind
    150000 10000
  ] def

  /video.modes.list [

    % add VBE modes >= 1024 x 576

    % first, try 16 bit
    /vm_color_bits 16 def

    {
      0 1 videomodes {
        videomodeinfo dup .undef eq {
          pop pop pop pop
        } {
          [
            over 0xbfff and 6 2 roll
            0x4000 and			% fb support
            exch vm_color_bits eq and	% color bits
            over 576 ge and		% height >= 576
            2 index 1024 ge and		% width >= 1024
          { 1 "" "" 5 -2 roll ] } { pop pop pop pop } ifelse
        } ifelse
      } for

      % no modes added? -> try 8 bit
      dup [ eq vm_color_bits 8 ne and {
        /vm_color_bits 8 def
      } {
        exit
      } ifelse
    } loop

    % add detected monitor size(s)

    video.res length {
      0 2 video.res length 1 sub {
        [
          exch 0 2 "" "" 5 -1 roll
          video.res over get exch video.res exch 1 add get
        ]
      } for
    } if

  ] def

  % sort video mode list

  video.modes.list length 3 gt {
    0 1 video.modes.list length 2 sub {
      dup 1 add 1 video.modes.list length 1 sub {
        over vmsortindex over vmsortindex gt {
          over over vmsortexch
        } if
        pop
      } for
      pop
    } for
  } if

  % remove duplicates

  /video.modes.list
    [
      /tmp_is_first true def
      video.modes.list {
        % if it's the 1st entry, just keep it
        % else, add a new entry only if different
        tmp_is_first {
          /tmp_is_first false def
        } {
          over .vm_width get over .vm_width get eq
          2 index .vm_height get 2 index .vm_height get eq and {
            over .vm_mode get over .vm_mode get max 2 index .vm_mode rot put
            over .vm_flags get over .vm_flags get or 2 index .vm_flags rot put
            free
          } if
        } ifelse
      } forall
    ]
    video.modes.list free
  def

  % create menu labels

  video.modes.list {
    dup .vm_label 32 string put
    dup .vm_height get over .vm_width get 2 index .vm_label get "%d x %d" exch dup length add sprintf

    pop
  } forall

  video.modes.list length {
    % add separating line before first entry
    video.modes.list 0 get
    .vm_label get dup strdup dup "\x01%s" 4 -1 roll sprintf free
  } if

  % build submenus

  /video.modes.installer [
    [ -1 0 ""           /txt_kernel_default 0 0 ]
    [ -2 0 "textmode=1" /txt_text_mode      0 0 ]

    video.modes.list { } forall
  ] def

  /video.modes.console [
    [ -1 0 ""           /txt_kernel_default 0 0 ]
    [ -2 0 "nomodeset"  /txt_video_no_kms   0 0 ]

    video.modes.list { } forall
  ] def

  /video.modes.bios [
    [ -1 0 ""           /txt_kernel_default 0 0 ]

    video.modes.list { } forall
  ] def

  /xmenu.video.installer .xm_size array def
  /xmenu.video.console .xm_size array def
  /xmenu.video.bios .xm_size array def

  % add to menu

  xmenu.video.installer .xm_list [ video.modes.installer { .vm_label get } forall ] put
  xmenu.video.installer .xm_title "Video Mode" put
  xmenu.video.installer .xm_current 0 put

  xmenu.video.console .xm_list [ video.modes.console { .vm_label get } forall ] put
  xmenu.video.console .xm_title "" put
  xmenu.video.console .xm_current 0 put

  xmenu.video.bios .xm_list [ video.modes.bios { .vm_label get } forall ] put
  xmenu.video.bios .xm_title "" put
  xmenu.video.bios .xm_current 0 put

  % create menu

  /xmenu.video.hc .xm_size array def
  xmenu.video.hc .xm_list [ "Default" "High Contrast" ] put
  xmenu.video.hc .xm_title "" put
  xmenu.video.hc .xm_current 0 put

  % add to menu

  /video.submenu.list [
    xmenu.video.installer
    xmenu.video.console
    xmenu.video.bios
    .undef
    xmenu.video.hc
    .undef
  ] def

  xmenu .xm_list [
    "Installer Size"
    "Text Console Size"
    "Video BIOS Size"
    "\x01Text Mode"
    "High Contrast"
    "Screen Reader"
  ] put

  % empty array
  /video.checkmarks xmenu .xm_list get length array def

  video.checkmarks 3 false put
  video.checkmarks 5 false put

  xmenu .xm_checkmarks video.checkmarks put

  xmenu .xm_submenus video.submenu.list put

  xmenu .xm_title /txt_video_mode put

  % 'default' entry
  xmenu .xm_current 0 put

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Update video mode.
%
% ( ) ==> ( )
%
/video.update {
  /xmenu xmenu.video def

  video.submenu.list xmenu .xm_current get get dup {
    video.submenu
    /window.action actRedrawPanel def
  } {
    pop
  } ifelse
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show video menu.
%
% ( ) => ( )
%
/panel.video {
  "videomode" help.setcontext

  window.xmenu
  dup .xmenu xmenu.video put
  dup .xmenu.update /video.update put
  dup window.init
      window.show
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Return width of video entry.
%
% ( ) => ( width )
%
/panel.video.width {
  /xmenu xmenu.video.installer def

  pmenu.width

  /xmenu xmenu.video def
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Redraw panel entry.
%
% ( panel ) => ( )
%
/panel.video.update {
  /xmenu xmenu.video.installer def

  pmenu.panel.update

  xmenu.video .xm_panel_x xmenu.video.installer .xm_panel_x get put

  /xmenu xmenu.video def
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Switch to submenu.
%
% ( submenu ) => ( )
%
/video.submenu {
  % set location of submenu
  dup .xm_panel_x xmenu.video .xm_panel_x get put

  "videomode" help.setcontext

  window.xmenu
  dup .xmenu 4 -1 roll put
  dup .xmenu.update /video.submenu.update put
  dup window.init
      window.show
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% What to do when submenu is closed.
%
% ( ) => ( )
%
/video.submenu.update {
  /window.action actRedrawPanel def
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Get current high contrast setting.
%
% ( ) => ( int )
%
/video.hc.current {
  xmenu.video.hc .xm_current get
} def

