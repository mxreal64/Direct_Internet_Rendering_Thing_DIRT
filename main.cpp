// dirt - direct internet rendering thing // real64 labs 2026
// copyright (c) 2026 [your name / handle]
//
// this program is free software: you can redistribute it and/or modify
// it under the terms of the gnu general public license as published by
// the free software foundation, either version 3 of the license, or
// (at your option) any later version.

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Scroll.H> 
#include <FL/fl_ask.H>
#include <curl/curl.h>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "core.h"
#include "parser.h"

void load_page(std::string url, bool add_to_history = true);

auto write_callback = [](void* contents, size_t size, size_t nmemb, void* user_data) -> size_t {
    size_t total_size = size * nmemb;
    auto* buffer = static_cast<std::string*>(user_data);
    buffer->append(static_cast<char*>(contents), total_size);
    return total_size;
};

bool download_binary_image(const std::string& url, const std::string& save_path) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    FILE* fp = fopen(save_path.c_str(), "wb");
    if (!fp) { curl_easy_cleanup(curl); return false; }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4L); 
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

class Dirt_Canvas : public Fl_Widget {
public:
    int total_calculated_height{600};

    Dirt_Canvas(int X, int Y, int W, int H, const char* L = nullptr) 
        : Fl_Widget(X, Y, W, H, L) {}

    void compute_layout() {
        int cur_x = 15;
        int cur_y = 25;
        fl_font(FL_HELVETICA, 14);
        int default_line_height = fl_height() + 4;
        int max_row_height = default_line_height;

        for (auto& node : dirt::g_ctx.layout_tree) {
            if (node.type == dirt::NodeType::Text || node.type == dirt::NodeType::InputElement) {
                node.width = fl_width(node.content.c_str());
                node.height = default_line_height;
            } 
            else if (node.type == dirt::NodeType::Image) {
                if (!node.img_ptr && !node.content.empty() && node.content.starts_with("http")) {
                    std::string tmp_file = "/tmp/dirt_img_" + std::to_string(std::hash<std::string>{}(node.content)) + ".jpg";
                    if (download_binary_image(node.content, tmp_file)) {
                        Fl_Shared_Image* raw_img = Fl_Shared_Image::get(tmp_file.c_str());
                        if (raw_img) {
                            double aspect = static_cast<double>(raw_img->h()) / raw_img->w();
                            int target_w = std::min(raw_img->w(), w() - 50);
                            int target_h = std::min(static_cast<int>(target_w * aspect), 180);
                            
                            node.img_ptr = static_cast<Fl_Shared_Image*>(raw_img->copy(target_w, target_h));
                            raw_img->release();
                        }
                    }
                }

                if (node.img_ptr) {
                    node.width = node.img_ptr->w();
                    node.height = node.img_ptr->h();
                } else {
                    node.width = 32;  
                    node.height = 20;
                }
            }

            if (cur_x + node.width > w() - 30) {
                cur_x = 15;
                cur_y += max_row_height + 8;
                max_row_height = default_line_height;
            }

            node.x = cur_x;
            node.y = cur_y;
            cur_x += node.width + 12;
            
            if (node.height > max_row_height) {
                max_row_height = node.height;
            }
        }
        total_calculated_height = cur_y + max_row_height + 50;
    }

    void update_widget_bounds() {
        compute_layout();
        if (h() != total_calculated_height) {
            resize(x(), y(), parent()->w() - 25, total_calculated_height);
        }
    }

    void draw() override {
        fl_color(FL_WHITE);
        fl_rectf(x(), y(), w(), h());

        fl_font(FL_HELVETICA, 14);

        for (const auto& node : dirt::g_ctx.layout_tree) {
            int render_x = x() + node.x;
            int render_y = y() + node.y;

            if (node.type == dirt::NodeType::Text) {
                if (!node.target_url.empty()) {
                    fl_color(FL_BLUE); 
                    fl_draw(node.content.c_str(), render_x, render_y + 14);
                    fl_line(render_x, render_y + 16, render_x + node.width, render_y + 16); 
                } else {
                    fl_color(FL_BLACK);
                    fl_draw(node.content.c_str(), render_x, render_y + 14);
                }
            } 
            else if (node.type == dirt::NodeType::InputElement) {
                fl_color(FL_LIGHT1);
                fl_rectf(render_x, render_y, node.width, node.height);
                fl_color(FL_BLACK);
                fl_draw(node.content.c_str(), render_x + 4, render_y + 15);
            } 
            else if (node.type == dirt::NodeType::Image) {
                if (node.img_ptr) {
                    node.img_ptr->draw(render_x, render_y); 
                } else {
                    fl_draw("🖼️", render_x, render_y + 14); 
                }
            }
        }
    }

    int handle(int event) override {
        int mx = Fl::event_x() - x();
        int my = Fl::event_y() - y();

        if (event == FL_MOVE) {
            bool over_link = false;
            for (const auto& node : dirt::g_ctx.layout_tree) {
                if (node.type == dirt::NodeType::Text && !node.target_url.empty()) {
                    if (mx >= node.x && mx <= node.x + node.width && my >= node.y && my <= node.y + node.height) {
                        window()->cursor(FL_CURSOR_HAND); 
                        over_link = true;
                        break;
                    }
                }
            }
            if (!over_link) window()->cursor(FL_CURSOR_DEFAULT);
            return 1;
        }

        if (event == FL_PUSH && Fl::event_button() == 1) {
            for (const auto& node : dirt::g_ctx.layout_tree) {
                if (mx >= node.x && mx <= node.x + node.width && my >= node.y && my <= node.y + node.height) {
                    if (node.type == dirt::NodeType::Text && !node.target_url.empty()) {
                        load_page(node.target_url);
                        return 1;
                    }
                }
            }
        }
        return Fl_Widget::handle(event);
    }
};

void load_page(std::string url, bool add_to_history) {
    if (url.empty()) return;
    if (!url.contains("://")) url = "https://" + url;

    if (url.starts_with("https:///wiki/")) {
        url = "https://wikipedia.org" + url.substr(8);
    }

    dirt::g_ctx.html_buffer.clear();
    dirt::g_ctx.current_page_url = url;
    dirt::g_ctx.url_input->value(url.c_str());
    Fl::check(); 

    CURL* curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dirt::g_ctx.html_buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 6L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && !dirt::g_ctx.html_buffer.empty()) {
        dirt::parse_html_to_tree(dirt::g_ctx.html_buffer);
        
        auto* canvas = static_cast<Dirt_Canvas*>(dirt::g_ctx.text_display);
        canvas->update_widget_bounds();
        
        auto* scroll_parent = static_cast<Fl_Scroll*>(canvas->parent());
        scroll_parent->scroll_to(0, 0);
        scroll_parent->redraw();
    }
}

void url_entered_cb(Fl_Widget*, void*) { load_page(dirt::g_ctx.url_input->value()); }

int main(int argc, char** argv) {
    fl_register_images(); 
    curl_global_init(CURL_GLOBAL_ALL);
    dirt::g_ctx.html_buffer.reserve(4 * 1024 * 1024);

    Fl_Window* window = new Fl_Window(1024, 768, "DIRT Browser Engine v4.0 (Real Images Edition)");

    Fl_Group* top_bar = new Fl_Group(0, 0, window->w(), 50);
    dirt::g_ctx.url_input = new Fl_Input(135, 15, window->w() - 230, 25, "Target Location:");
    dirt::g_ctx.url_input->value("https://wikipedia.org");
    dirt::g_ctx.url_input->callback(url_entered_cb);

    Fl_Button* go_button = new Fl_Button(window->w() - 85, 15, 75, 25, "Render");
    go_button->callback(url_entered_cb);
    top_bar->end();

    Fl_Scroll* scroll_container = new Fl_Scroll(0, 50, window->w(), window->h() - 50);
    scroll_container->type(Fl_Scroll::VERTICAL);
    
    dirt::g_ctx.text_display = new Dirt_Canvas(0, 50, window->w() - 25, window->h() - 50);
    scroll_container->end();

    window->resizable(scroll_container);
    window->end();
    
    window->show(argc, argv);
    window->maximize();

    load_page(dirt::g_ctx.url_input->value());
    int ret = Fl::run();
    curl_global_cleanup();
    return ret;
}
