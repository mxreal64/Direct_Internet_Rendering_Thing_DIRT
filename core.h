// dirt - direct internet rendering thing // real64 labs 2026
// copyright (c) 2026 [your name / handle]
//
// this program is free software: you can redistribute it and/or modify
// it under the terms of the gnu general public license as published by
// the free software foundation, either version 3 of the license, or
// (at your option) any later version.

#pragma once
#include <string>
#include <vector>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Shared_Image.H>

namespace dirt {
    enum class NodeType { Text, Image, InputElement };

    struct LayoutNode {
        NodeType type;
        std::string content;      
        std::string target_url;   
        std::string input_name;   
        int x{0}, y{0}, width{0}, height{0};
        Fl_Shared_Image* img_ptr{nullptr}; 
    };

    struct alignas(64) BrowserContext {
        std::string html_buffer;
        std::string current_page_url;
        std::vector<std::string> history_stack;
        std::vector<LayoutNode> layout_tree; 
        Fl_Widget* text_display{nullptr}; 
        Fl_Input* url_input{nullptr};
        Fl_Button* back_button{nullptr};
        std::string focused_input_name;
        std::string active_form_action;
        bool show_cursor{true};
    };

    inline BrowserContext g_ctx;
}
