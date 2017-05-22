#if !defined(FCODER_EMACS)
#define FCODER_EMACS

//states
static bool IsModal = false;
static bool HasDefaultBarColor = false;

static int_color DefaultBarColor = 0;
static int_color DefaultMarginColor = 0;
static int_color ModalBarColor = 0x964421;
static int_color ModalMarginColor = 0x964421;

inline int_color
GetThemeColor(Application_Links *app, Style_Tag tag)
{
    int_color Result = 0;
    
    Theme_Color Color = {};
    Color.tag = tag;
    get_theme_colors(app, &Color, 1);

    Result = Color.color;
    return Result;
}

inline void
SetThemeColor(Application_Links *app, Style_Tag tag, int_color Color)
{
    Theme_Color ThemeColor = {tag, Color};
    set_theme_colors(app, &ThemeColor, 1);
}

CUSTOM_COMMAND_SIG(EnterModal)
{
    IsModal = true;

    if (!HasDefaultBarColor)
    {
        HasDefaultBarColor = true;
        
        DefaultBarColor = GetThemeColor(app, Stag_Bar);
        DefaultMarginColor = GetThemeColor(app, Stag_Margin_Active);
    }

    SetThemeColor(app, Stag_Bar, ModalBarColor);
    SetThemeColor(app, Stag_Margin_Active, ModalMarginColor);
}

CUSTOM_COMMAND_SIG(LeaveModal)
{
    IsModal = false;
    SetThemeColor(app, Stag_Bar, DefaultBarColor);
    SetThemeColor(app, Stag_Margin_Active, DefaultMarginColor);
}    

#define MODAL(FunctionName) command_##FunctionName
#define DEFINE_MODAL(FunctionName, Key)                                     \
CUSTOM_COMMAND_SIG(MODAL(FunctionName)) {                           \
    if (IsModal)                                                    \
    {                                                               \
        FunctionName(app);                                          \
        LeaveModal(app);                                               \
    }                                                               \
    else                                                            \
    {                                                               \
        exec_command(app, write_character);                         \
    }                                                               \
}

DEFINE_MODAL(open_panel_vsplit, '3')
DEFINE_MODAL(close_panel, '0')
DEFINE_MODAL(change_active_panel, 'o')
DEFINE_MODAL(interactive_new, 'n')
DEFINE_MODAL(interactive_open, 'f')
DEFINE_MODAL(kill_buffer, 'k')

CUSTOM_COMMAND_SIG(GotoBeginOfFile)
{
    View_Summary ActiveView = get_active_view(app, AccessOpen|AccessProtected);
    
    Buffer_Seek NewCursorPos = {};
    NewCursorPos.type = buffer_seek_pos;
    NewCursorPos.pos = 0;
    
    view_set_cursor(app, &ActiveView, NewCursorPos, true);
}

CUSTOM_COMMAND_SIG(GotoEndOfFile)
{
    View_Summary ActiveView = get_active_view(app, AccessOpen|AccessProtected);
    Buffer_Summary CurrentBuffer = get_buffer(app, ActiveView.buffer_id, AccessOpen|AccessProtected);
    
    Buffer_Seek NewCursorPos = {};
    NewCursorPos.type = buffer_seek_pos;
    NewCursorPos.pos = CurrentBuffer.size; 
    
    view_set_cursor(app, &ActiveView, NewCursorPos, true);
}

CUSTOM_COMMAND_SIG(KillLine)
{
    //NOTE(chen): accomplished by marking curent pos, mvoe cursor to the end of line, then delete range
    //TODO(chen): DO THIS
    View_Summary View = get_active_view(app, AccessOpen|AccessProtected);
    Buffer_Summary Buffer = get_buffer(app, View.buffer_id, AccessOpen|AccessProtected);
    
    Buffer_Seek CurrentPosSeek = {};
    CurrentPosSeek.type = buffer_seek_pos;
    CurrentPosSeek.pos = View.cursor.pos;
    int32_t LineEndPos = seek_line_end(app, &Buffer, View.cursor.pos);
    
    if (view_set_mark(app, &View, CurrentPosSeek))
    {
        Buffer_Seek LineEndPosSeek = {};
        LineEndPosSeek.type = buffer_seek_pos;
        LineEndPosSeek.pos = LineEndPos;
        
        if (LineEndPos == CurrentPosSeek.pos) //NOTE(chen): if there's just the \n char, delete the \n then
        {
            LineEndPosSeek.pos += 1; 
        }
        
        if (view_set_cursor(app, &View, LineEndPosSeek, true))
        {
            delete_range(app);
        }
    }
}

CUSTOM_COMMAND_SIG(SetDOSMode)
{
    View_Summary View = get_active_view(app, AccessOpen|AccessProtected);
    Buffer_Summary Buffer = get_buffer(app, View.buffer_id, AccessOpen|AccessProtected);
    
    buffer_set_setting(app, &Buffer, BufferSetting_Eol, 1);
}

void emacs_keys(Bind_Helper *context){
    begin_map(context, mapid_global);
    
    /*chen's unique commands*/
    bind(context, '<', MDFR_ALT, GotoBeginOfFile);
    bind(context, '>', MDFR_ALT, GotoEndOfFile);
    bind(context, 'k', MDFR_CTRL, KillLine);
    bind(context, '1', MDFR_CTRL, SetDOSMode);
    
    /*modal enter/leave*/
    bind(context, 'x', MDFR_CTRL, EnterModal);
    bind(context, 'g', MDFR_CTRL, LeaveModal);
    
    //    bind(context, 'p', MDFR_CTRL, open_panel_vsplit);
    bind(context, '_', MDFR_CTRL, open_panel_hsplit);
    //    bind(context, 'P', MDFR_CTRL, close_panel);
    //    bind(context, ',', MDFR_CTRL, change_active_panel);
    
    //    bind(context, 'n', MDFR_CTRL, interactive_new);
    //    bind(context, 'o', MDFR_CTRL, interactive_open);
    bind(context, 'o', MDFR_ALT, open_in_other);
    //    bind(context, 'k', MDFR_CTRL, interactive_kill_buffer);
    bind(context, 'i', MDFR_CTRL, interactive_switch_buffer);
    bind(context, 'w', MDFR_CTRL, save_as);
    
    bind(context, 'c', MDFR_ALT, open_color_tweaker);
    bind(context, 'd', MDFR_ALT, open_debug);
    
    bind(context, '.', MDFR_ALT, change_to_build_panel);
    bind(context, ',', MDFR_ALT, close_build_panel);
    bind(context, 'n', MDFR_ALT, goto_next_error);
    bind(context, 'N', MDFR_ALT, goto_prev_error);
    bind(context, 'M', MDFR_ALT, goto_first_error);
    bind(context, 'm', MDFR_ALT, build_in_build_panel);
    
    bind(context, 'z', MDFR_ALT, execute_any_cli);
    bind(context, '`', MDFR_ALT, execute_previous_cli);
    
    bind(context, 'x', MDFR_ALT, execute_arbitrary_command);
    
#if 0    
    bind(context, 's', MDFR_ALT, show_scrollbar);
    bind(context, 'w', MDFR_ALT, hide_scrollbar);
#endif
    
    // TODO(allen): This is apparently not working on Linux, must investigate.
    bind(context, key_f2, MDFR_CTRL, toggle_mouse);
    bind(context, key_page_up, MDFR_ALT, toggle_fullscreen);
    bind(context, 'E', MDFR_ALT, exit_4coder);
    
    bind(context, key_f1, MDFR_NONE, project_fkey_command);
    bind(context, key_f2, MDFR_NONE, project_fkey_command);
    bind(context, key_f3, MDFR_NONE, project_fkey_command);
    bind(context, key_f4, MDFR_NONE, project_fkey_command);
    
    bind(context, key_f5, MDFR_NONE, project_fkey_command);
    bind(context, key_f6, MDFR_NONE, project_fkey_command);
    bind(context, key_f7, MDFR_NONE, project_fkey_command);
    bind(context, key_f8, MDFR_NONE, project_fkey_command);
    
    bind(context, key_f9, MDFR_NONE, project_fkey_command);
    bind(context, key_f10, MDFR_NONE, project_fkey_command);
    bind(context, key_f11, MDFR_NONE, project_fkey_command);
    bind(context, key_f12, MDFR_NONE, project_fkey_command);
    
    bind(context, key_f13, MDFR_NONE, project_fkey_command);
    bind(context, key_f14, MDFR_NONE, project_fkey_command);
    bind(context, key_f15, MDFR_NONE, project_fkey_command);
    bind(context, key_f16, MDFR_NONE, project_fkey_command);
    
    end_map(context);
    
    begin_map(context, default_code_map);
    
    // NOTE(allen|a3.1): Set this map (default_code_map == mapid_user_custom) to
    // inherit from mapid_file.  When searching if a key is bound
    // in this map, if it is not found here it will then search mapid_file.
    //
    // If this is not set, it defaults to mapid_global.
    inherit_map(context, mapid_file);
    
    // NOTE(allen|a3.1): Children can override parent's bindings.
    bind(context, 'f', MDFR_ALT, seek_alphanumeric_or_camel_right);
    bind(context, 'b', MDFR_ALT, seek_alphanumeric_or_camel_left);
    
    // NOTE(allen|a3.2): Specific keys can override vanilla keys,
    // and write character writes whichever character corresponds
    // to the key that triggered the command.
    bind(context, '\n', MDFR_NONE, write_and_auto_tab);
    bind(context, '\n', MDFR_SHIFT, write_and_auto_tab);
    // NOTE: bind(context, '}', MDFR_NONE, write_and_auto_tab);
    bind(context, ')', MDFR_NONE, write_and_auto_tab);
    bind(context, ']', MDFR_NONE, write_and_auto_tab);
    bind(context, ';', MDFR_NONE, write_and_auto_tab);
    bind(context, '#', MDFR_NONE, write_and_auto_tab);
    
    bind(context, '\t', MDFR_NONE, word_complete);
    bind(context, '\t', MDFR_CTRL, auto_tab_range);
    bind(context, '\t', MDFR_SHIFT, auto_tab_line_at_cursor);
    
    bind(context, 't', MDFR_ALT, write_todo);
    bind(context, 'y', MDFR_ALT, write_note);
    bind(context, 'r', MDFR_ALT, write_block);
    bind(context, '[', MDFR_CTRL, open_long_braces);
    bind(context, '{', MDFR_CTRL, open_long_braces_semicolon);
    bind(context, '}', MDFR_CTRL, open_long_braces_break);
    bind(context, 'i', MDFR_ALT, if0_off);
    bind(context, '1', MDFR_ALT, open_file_in_quotes);
    bind(context, '2', MDFR_ALT, open_matching_file_cpp);
    bind(context, '0', MDFR_CTRL, write_zero_struct);
    bind(context, 'I', MDFR_CTRL, list_all_functions_current_buffer);
    
    end_map(context);
    
    
    begin_map(context, mapid_file);
    
    // NOTE(allen|a3.4.4): Binding this essentially binds
    // all key combos that would normally insert a character
    // into a buffer. If the code for the key is not an enum
    // value such as key_left or key_back then it is a vanilla key.
    // It is possible to override this binding for individual keys.
    bind_vanilla_keys(context, write_character);
    
    /*@Modal keybinding*/
    bind(context, '3', MDFR_NONE, MODAL(open_panel_vsplit));
    bind(context, 'o', MDFR_NONE, MODAL(change_active_panel));
    bind(context, '0', MDFR_NONE, MODAL(close_panel));
    bind(context, 'f', MDFR_NONE, MODAL(interactive_open));
    bind(context, 'n', MDFR_NONE, MODAL(interactive_new));
    bind(context, 'k', MDFR_NONE, MODAL(kill_buffer));
    
    // NOTE(allen|a4.0.7): You can now bind left and right clicks.
    // They only trigger on mouse presses.  Modifiers do work
    // so control+click shift+click etc can now have special meanings.
    bind(context, key_mouse_left, MDFR_NONE, click_set_cursor);
    bind(context, key_mouse_right, MDFR_NONE, click_set_mark);
    
    // NOTE(allen|a4.0.11): You can now bind left and right mouse
    // button releases.  Modifiers do work so control+click shift+click
    // etc can now have special meanings.
    bind(context, key_mouse_left_release, MDFR_NONE, click_set_mark);
    
    bind(context, 'b', MDFR_CTRL, move_left);
    bind(context, 'f', MDFR_CTRL, move_right);
    bind(context, key_del, MDFR_NONE, delete_char);
    bind(context, key_del, MDFR_SHIFT, delete_char);
    bind(context, key_back, MDFR_NONE, backspace_char);
    bind(context, key_back, MDFR_SHIFT, backspace_char);
    bind(context, 'p', MDFR_CTRL, move_up);
    bind(context, 'n', MDFR_CTRL, move_down);
    bind(context, 'e', MDFR_CTRL, seek_end_of_line);
    bind(context, 'a', MDFR_CTRL, seek_beginning_of_line);
    bind(context, 'v', MDFR_ALT, page_up);
    bind(context, 'v', MDFR_CTRL, page_down);
    bind(context, key_page_up, MDFR_NONE, page_up);
    bind(context, key_page_down, MDFR_NONE, page_down);
    
    bind(context, 'f', MDFR_ALT, seek_whitespace_right);
    bind(context, 'b', MDFR_ALT, seek_whitespace_left);
    bind(context, '{', MDFR_ALT, seek_whitespace_up_end_line);
    bind(context, '}', MDFR_ALT, seek_whitespace_down_end_line);
    
    bind(context, key_up, MDFR_ALT, move_up_10);
    bind(context, key_down, MDFR_ALT, move_down_10);
    
    bind(context, key_back, MDFR_CTRL, backspace_word);
    bind(context, key_del, MDFR_CTRL, delete_word);
    bind(context, key_back, MDFR_ALT, snipe_token_or_word);
    
    bind(context, ' ', MDFR_CTRL, set_mark);
    //    bind(context, 'a', MDFR_CTRL, replace_in_range);
    bind(context, 'w', MDFR_ALT, copy); //TODO(chen): why the hell is this not working? investigate
    bind(context, 'd', MDFR_CTRL, delete_range);
    bind(context, 'l', MDFR_CTRL, center_view);
    bind(context, 'E', MDFR_CTRL, left_adjust_view);
    bind(context, 's', MDFR_CTRL, search);
    bind(context, 'S', MDFR_CTRL, list_all_locations);
    bind(context, 'S', MDFR_ALT, list_all_substring_locations_case_insensitive);
    bind(context, 'g', MDFR_ALT, goto_line);
    bind(context, 'j', MDFR_CTRL, to_lowercase);
    //    bind(context, 'K', MDFR_CTRL, kill_buffer);
    //    bind(context, 'l', MDFR_CTRL, toggle_line_wrap);
    bind(context, 'm', MDFR_CTRL, cursor_mark_swap);
    bind(context, 'O', MDFR_CTRL, reopen);
    bind(context, 'q', MDFR_CTRL, query_replace);
    bind(context, 'r', MDFR_CTRL, reverse_search);
    bind(context, 's', MDFR_ALT, save);
    bind(context, 'T', MDFR_CTRL, list_all_locations_of_identifier);
    bind(context, 'u', MDFR_CTRL, to_uppercase);
    bind(context, 'y', MDFR_CTRL, paste_and_indent);
    bind(context, 'v', MDFR_ALT, toggle_virtual_whitespace);
    bind(context, 'V', MDFR_CTRL, paste_next_and_indent);
    bind(context, 'w', MDFR_CTRL, cut);
    bind(context, 'y', MDFR_CTRL, redo);
    bind(context, 'z', MDFR_CTRL, undo);
    
    bind(context, '2', MDFR_CTRL, decrease_line_wrap);
    bind(context, '3', MDFR_CTRL, increase_line_wrap);
    
    bind(context, '?', MDFR_CTRL, toggle_show_whitespace);
    bind(context, '~', MDFR_CTRL, clean_all_lines);
    bind(context, '\n', MDFR_NONE, newline_or_goto_position);
    bind(context, '\n', MDFR_SHIFT, newline_or_goto_position);
    bind(context, ' ', MDFR_SHIFT, write_character);
    
    end_map(context);
}

#endif

/*
Modal:

c-x enter
c-g leave
*/

/* Modal stuff:

c-x o -> switch pannel
c-x 3 -> panel vsplit
c-x 0 -> panel close

c-x n -> open new file
c-x f -> interactive open
c-x k -> kill buffer
*/

/* Modified keys:

m-s   -> save

all movement code
begin_of_line and end_of_line
page up, page down

alt-g  -> goto-line
alt-w  -> copy
ctr-w  -> cut

ctr-l  -> center view

ctr-s  -> search
ctr-S  -> search in all location
alt-S  -> list all substring
ctr-y  -> paste
ctr-_  -> undo
*/

/*
New Stuff:

ctr-k   -> kill line

*/
