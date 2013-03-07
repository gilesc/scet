// Simplified implementation of Schwarz' extractAbbrev
#include <string>
#include <vector>
#include <iostream>

using namespace std;

struct Acronym {
    string short_form;
    string long_form;
};

vector<string> split_sentences(const string& text) {
    vector<string> sentences;
    int start = 0;
    int end = 0;
    while ((end = text.find(". ", start)) != string::npos) {
        sentences.push_back(text.substr(start, end-start));
        start = end + 2;
    }
    if (end != text.size())
        sentences.push_back(text.substr(start, text.size()));
    return sentences;
}

void trim(string& s) {
    size_t end = s.find_last_not_of(" \t\n");
    if (end != string::npos)
        s = s.substr(0, end+1);
    size_t start = s.find_first_not_of(" \t\n");
    if (start != string::npos)
        s = s.substr(start);
}

struct CharRange {
    int start, end;

    CharRange(int _start, int _end)  : start(_start), end(_end) {}

    bool operator()(string& s) {
        for (char& c : s)
            if (operator()(c))
                return true;
        return false;
    }

    bool operator()(char c) {
        return (start <= c) && (c <= end);
    }
};

CharRange has_capital(65,90);
CharRange has_letter(65,122);
CharRange has_digit(48,57);

bool valid_short_form(string& s) {
    return (s.find(" ")==string::npos) && (s.size() > 1) 
        && has_letter(s) && (has_letter(s[0]) || has_digit(s[0]));
}

string find_best_long_form(const string& short_form, const string& long_form) {
    int s_ix = short_form.size() - 1;
    int l_ix = long_form.size() - 1;
    char c;
    while (s_ix--) {
        c = tolower(short_form[s_ix]);
        if (!(has_letter(c) || has_digit(c)))
            continue;
        while (((l_ix>=0) && (tolower(long_form[l_ix]) != c)) ||
               ((s_ix==0) && (l_ix > 0) && (has_letter(long_form[l_ix-1]) ||
                                            has_digit(long_form[l_ix-1]))))
            l_ix--;
        if (l_ix < 0)
            return "";
    }
    l_ix = long_form.rfind(" ", l_ix) + 1;
    if ((long_form.size() - l_ix) > 50)
        return "";
    return long_form.substr(l_ix);
}

vector<Acronym> extract_acronyms(const string& text) {
    vector<string> sentences = split_sentences(text);
    vector<Acronym> acronyms;
    for (string sentence : sentences) {
        int start, oparen, cparen;
        start = 0;
        while (((oparen = sentence.find("(", start)) != string::npos)
               && ((cparen = sentence.find(")", oparen)) != string::npos)) {
            string short_form = sentence.substr(oparen+1, cparen-oparen-1);
            string long_form = sentence.substr(start, oparen-start);
            if (long_form.size() < short_form.size()) {
                string tmp = long_form;
                long_form = short_form;
                short_form = tmp;
            }
            if (valid_short_form(short_form) &&
                    ((long_form = find_best_long_form(short_form, long_form)).size())) {
                trim(long_form);
                if (short_form != long_form)
                    acronyms.push_back(Acronym{short_form, long_form});
            }
            start = cparen+1;
        }
    }
    return acronyms;
}

void replace_acronyms_with_long_forms(string& text) {
    vector<Acronym> acronyms = extract_acronyms(text);
    for (Acronym ac : acronyms) {
        int start = 0;
        int end;
        while ((start = text.find("("+ac.short_form+")", start)) != string::npos) {
            end = start + 2 + ac.short_form.size();
            text = text.substr(0, start) + "( " + ac.long_form + " )" + text.substr(end);
            start = end;
        }
    }
}

/*
int main(int argc, char* argv[]) {
    string line;
    while (getline(cin, line)) {
        replace_acronyms_with_long_forms(line); 
        cout << line << endl;
        vector<Acronym> acronyms = extract_acronyms(line);
        for (Acronym ac : acronyms) {
            cout << ac.short_form << "\t|||\t" << ac.long_form << endl;
        }
    }
}
*/
