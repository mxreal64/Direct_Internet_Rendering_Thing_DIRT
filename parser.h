// dirt - direct internet rendering thing // real64 labs 2026
// copyright (c) 2026 [your name / handle]
//
// this program is free software: you can redistribute it and/or modify
// it under the terms of the gnu general public license as published by
// the free software foundation, either version 3 of the license, or
// (at your option) any later version.

#pragma once
#include <string>
#include <string_view>
#include <ranges>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <vector>
#include "core.h"

namespace dirt {
    inline bool match_tag_inline(std::string_view html, uint64_t pos, std::string_view target) {
        if (pos + target.size() > html.size()) return false;
        return std::ranges::equal(html.substr(pos, target.size()), target, [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        });
    }

    inline std::string extract_attribute(std::string_view tag, std::string_view attr_name) {
        size_t pos = tag.find(attr_name);
        if (pos == std::string::npos) return "";
        pos = tag.find('=', pos);
        if (pos == std::string::npos) return "";
        pos = tag.find('"', pos);
        if (pos == std::string::npos) return "";
        size_t end_pos = tag.find('"', pos + 1);
        if (end_pos == std::string::npos) return "";
        return std::string(tag.substr(pos + 1, end_pos - pos - 1));
    }

    inline void parse_html_to_tree(std::string_view source) {
        g_ctx.layout_tree.clear();
        bool in_tag{false};
        bool skip_entire_block{false};
        std::string_view active_block_tag{""};
        std::string current_link_target{""};
        std::string current_text_accumulator{""};

        auto flush_text = [&]() {
            if (!current_text_accumulator.empty() && !skip_entire_block) {
                if (current_text_accumulator.find_first_not_of(" \t\n\r") != std::string::npos) {
                    LayoutNode node{NodeType::Text, current_text_accumulator, current_link_target};
                    g_ctx.layout_tree.push_back(node);
                }
                current_text_accumulator.clear();
            }
        };

        for (size_t idx{0}; idx < source.size(); ++idx) {
            char c = source[idx];

            if (c == '<') {
                flush_text();
                in_tag = true;

                if (!skip_entire_block) {
                    if (match_tag_inline(source, idx + 1, "script")) { 
                        skip_entire_block = true; 
                        active_block_tag = "/script"; 
                        idx += 6; 
                        continue; 
                    }
                    if (match_tag_inline(source, idx + 1, "style"))  { 
                        skip_entire_block = true; 
                        active_block_tag = "/style"; 
                        idx += 5; 
                        continue; 
                    }

                    if (match_tag_inline(source, idx + 1, "a")) {
                        size_t close = source.find('>', idx);
                        if (close != std::string::npos) {
                            std::string href = extract_attribute(source.substr(idx, close - idx), "href");
                            if (!href.empty()) {
                                // CRITICAL FIX: Resolve relative links down to absolute URLs
                                if (href.starts_with("//")) {
                                    href = "https:" + href;
                                } else if (href.starts_with("/")) {
                                    size_t proto_end = g_ctx.current_page_url.find("://");
                                    size_t slash_end = g_ctx.current_page_url.find('/', proto_end + 3);
                                    std::string base = (slash_end == std::string::npos) ? g_ctx.current_page_url : g_ctx.current_page_url.substr(0, slash_end);
                                    href = base + href;
                                } else if (!href.starts_with("http") && !href.starts_with("javascript:")) {
                                    size_t last_slash = g_ctx.current_page_url.find_last_of('/');
                                    if (last_slash > 8) {
                                        href = g_ctx.current_page_url.substr(0, last_slash + 1) + href;
                                    } else {
                                        href = g_ctx.current_page_url + "/" + href;
                                    }
                                }
                                current_link_target = href;
                            }
                        }
                    }
                    if (match_tag_inline(source, idx + 1, "/a")) { current_link_target.clear(); }

                    if (match_tag_inline(source, idx + 1, "input")) {
                        size_t close = source.find('>', idx);
                        if (close != std::string::npos) {
                            std::string name = extract_attribute(source.substr(idx, close - idx), "name");
                            if (!name.empty()) {
                                LayoutNode node{NodeType::InputElement, " [Field: " + name + "] "};
                                node.input_name = name;
                                g_ctx.layout_tree.push_back(node);
                            }
                        }
                    }

                    if (match_tag_inline(source, idx + 1, "img")) {
                        size_t close = source.find('>', idx);
                        if (close != std::string::npos) {
                            std::string_view tag = source.substr(idx, close - idx);
                            std::string src = extract_attribute(tag, "src");
                            std::string alt = extract_attribute(tag, "alt");
                            if (alt.empty()) alt = "Image";

                            LayoutNode node{NodeType::Image, alt, current_link_target};
                            if (!src.starts_with("http") && !src.empty()) {
                                if (src.starts_with("//")) {
                                    src = "https:" + src;
                                } else if (src.starts_with("/")) {
                                    size_t proto_end = g_ctx.current_page_url.find("://");
                                    size_t slash_end = g_ctx.current_page_url.find('/', proto_end + 3);
                                    std::string base = (slash_end == std::string::npos) ? g_ctx.current_page_url : g_ctx.current_page_url.substr(0, slash_end);
                                    src = base + src;
                                } else {
                                    size_t last_slash = g_ctx.current_page_url.find_last_of('/');
                                    if (last_slash > 8) {
                                        src = g_ctx.current_page_url.substr(0, last_slash + 1) + src;
                                    } else {
                                        src = g_ctx.current_page_url + "/" + src;
                                    }
                                }
                            }
                            node.content = src; 
                            g_ctx.layout_tree.push_back(node);
                        }
                    }
                } else {
                    if (match_tag_inline(source, idx + 1, active_block_tag)) {
                        idx += active_block_tag.size();
                        size_t close = source.find('>', idx);
                        if (close != std::string::npos) idx = close;
                        skip_entire_block = false;
                        active_block_tag = "";
                        in_tag = false;
                    }
                }
                continue;
            }

            if (c == '>') { in_tag = false; continue; }

            if (!in_tag && !skip_entire_block) {
                if (std::isspace(static_cast<unsigned char>(c))) {
                    if (!current_text_accumulator.empty() && current_text_accumulator.back() != ' ') {
                        current_text_accumulator.push_back(' ');
                    }
                } else {
                    current_text_accumulator.push_back(c);
                }
            }
        }
        flush_text();
    }
}
