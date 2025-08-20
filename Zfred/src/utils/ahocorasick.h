#pragma once

#include <vector>
#include <queue>
#include <string_view>
#include <memory>      // for unique_ptr
#include <algorithm>   // for all_of, find_if
#include <cstddef>

template<typename CharT>
class AhoCorasick {
public:
    using StringView = std::basic_string_view<CharT>;

    struct Node {
        // Use small-vector optimization if needed for larger alphabets
        struct Edge {
            CharT ch;
            Node* next;
        };
        std::vector<Edge> nexts;
        Node* fail = nullptr;
        int pat_idx = -1;
    };

    // For memory efficiency, store nodes with unique_ptrs
    std::vector<std::unique_ptr<Node>> all_nodes;
    Node* root;

    AhoCorasick() {
        all_nodes.emplace_back(std::make_unique<Node>());
        root = all_nodes.back().get();
    }

    void build(const std::vector<StringView>& patterns) {
        // Trie construction
        for (int i = 0; i < static_cast<int>(patterns.size()); ++i) {
            Node* node = root;
            for (CharT c : patterns[i]) {
                auto it = std::find_if(node->nexts.begin(), node->nexts.end(),
                    [c](const typename Node::Edge& e) { return e.ch == c; });
                if (it == node->nexts.end()) {
                    all_nodes.emplace_back(std::make_unique<Node>());
                    //node->nexts.push_back(Edge{ c, all_nodes.back().get() });
                    node->nexts.push_back(typename Node::Edge{ c, all_nodes.back().get() });
                    node = all_nodes.back().get();
                }
                else {
                    node = it->next;
                }
            }
            node->pat_idx = i;
        }
        // Build failure links (BFS)
        std::queue<Node*> q;
		for (typename Node::Edge& e : root->nexts){
            e.next->fail = root;
            q.push(e.next);
        }
        while (!q.empty()) {
            Node* u = q.front(); q.pop();
            for (typename Node::Edge& e : u->nexts) {
                Node* f = u->fail;
                while (f && !has_edge(f, e.ch))
                    f = f->fail;
                if (f)
                    e.next->fail = get_edge(f, e.ch);
                else
                    e.next->fail = root;
                q.push(e.next);
            }
        }
    }
    void search(const StringView& text, std::vector<bool>& found) const {
        Node* node = root;
        for (CharT c : text) {
            while (node && !has_edge(node, c))
                node = node->fail;
            if (node)
                node = get_edge(node, c);
            else
                node = root;
            for (Node* tmp = node; tmp && tmp->pat_idx != -1; tmp = tmp->fail)
                found[tmp->pat_idx] = true;
        }
    }

private:
    static bool has_edge(Node* n, CharT c) {
        for (const auto& e : n->nexts) if (e.ch == c) return true;
        return false;
    }
    static Node* get_edge(Node* n, CharT c) {
        for (const auto& e : n->nexts) if (e.ch == c) return e.next;
        return nullptr;
    }
};

// for example
template<typename CharT>
bool ah_substring_match(const std::vector<std::basic_string_view<CharT>>& patterns,
    std::basic_string_view<CharT> str)
{
    if (patterns.empty())
        return true; // Or false, depending on requirements
    AhoCorasick<CharT> ac;
    ac.build(patterns);
    std::vector<bool> found(patterns.size(), false);
    ac.search(str, found);
    // Return true only if every pattern found at least once
    return std::all_of(found.begin(), found.end(), [](bool b) {return b; });
}

//// Example Usage:
//#include <iostream>
//int main() {
//    using sv = std::string_view;
//    std::vector<sv> pats = { "the", "cat", "on" };
//    sv txt = "the cat sat on the mat";
//    bool res = ah_substring_match(pats, txt);
//    std::cout << res << '\n'; // prints 1 (true)
//
//    // Works for wstring_view too!
//    std::vector<std::wstring_view> wpats = { L"Ã¨", L"×ø" };
//    std::wstring wtxt = L"Ã¨×øÔÚµæ×ÓÉÏ";
//    bool wres = substring_match(wpats, std::wstring_view(wtxt));
//    std::wcout << wres << L'\n'; // prints 1 (true)
//}



//#include <vector>
//#include <queue>
//#include <string>
//#include <unordered_map>
//using namespace std;
//
//// The assumption is: String == std::string
//using String = std::wstring;
//
//struct TrieNode {
//    unordered_map<char, TrieNode*> next;
//    TrieNode* fail = nullptr;
//    int pat_idx = -1; // index of pattern ending here, or -1
//};
//
//class AhoCorasick {
//public:
//    TrieNode root;
//    // patterns[i] => pattern index, needed to check matches
//    void build(const vector<String>& patterns) {
//        // Build trie
//        for (int i = 0; i < patterns.size(); ++i) {
//            TrieNode* node = &root;
//            for (char c : patterns[i]) {
//                if (!node->next.count(c))
//                    node->next[c] = new TrieNode();
//                node = node->next[c];
//            }
//            node->pat_idx = i;
//        }
//
//        // Build fail links
//        queue<TrieNode*> q;
//        for (auto& kv : root.next) {
//            kv.second->fail = &root;
//            q.push(kv.second);
//        }
//        while (!q.empty()) {
//            TrieNode* u = q.front(); q.pop();
//            for (auto& kv : u->next) {
//                char c = kv.first;
//                TrieNode* v = kv.second;
//                TrieNode* f = u->fail;
//                while (f && !f->next.count(c))
//                    f = f->fail;
//                if (f) v->fail = f->next[c];
//                else v->fail = &root;
//                q.push(v);
//            }
//        }
//    }
//
//    // Find all pattern indices found during search
//    void search(const String& s, vector<bool>& pat_found) const {
//        TrieNode* node = const_cast<TrieNode*>(&root);
//        for (char c : s) {
//            while (node && !node->next.count(c))
//                node = node->fail;
//            if (node) node = node->next[c];
//            else node = const_cast<TrieNode*>(&root);
//            TrieNode* temp = node;
//            while (temp && temp->pat_idx != -1) {
//                pat_found[temp->pat_idx] = true;
//                temp = temp->fail;
//            }
//        }
//    }
//};

//bool substring_match(const vector<String>& patterns, const String& str) {
//    if (patterns.empty())
//        return true; // or false, depending on definition
//
//    AhoCorasick ac;
//    ac.build(patterns);
//
//    vector<bool> found(patterns.size(), false);
//    ac.search(str, found);
//
//    // Return true only if every pattern found at least once
//    for (bool b : found)
//        if (!b) return false;
//    return true;
//}