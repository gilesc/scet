#include <string>
#include <vector>
#include <memory>
using namespace std;

namespace ahocorasick {

class Node {
public:
	Node() {};
	Node(char c) {content = c; terminal = false;}
	int id;
	int depth();
	char content;		
	bool terminal;
	vector<shared_ptr<Node> > children;
	shared_ptr<Node> parent;
	shared_ptr<Node> fail;
	shared_ptr<Node> find(char c);
	shared_ptr<Node> find_or_fail(char c);
};

struct Match {
	int id;
	int start;
	int end;
	int length() const {return end - start;}
	bool overlaps(const Match& o) const {
		return end <= o.start && start < o.end;
	}
};

struct Settings {
	bool case_sensitive;
	bool remove_overlaps;
	bool break_on_word_boundaries;
	Settings() : case_sensitive(false), remove_overlaps(false),
		break_on_word_boundaries(true) {};
};

class Trie {
public:
	Trie();
	Trie(Settings& s);
	vector<Match> search(string s);
	void add(string s);
	void add(int id, string s);
	void build();
private:
	shared_ptr<Node> root;
	Settings settings;
	void add_fail_transitions(shared_ptr<Node> n);
    int n_words;
};

vector<Match> remove_overlaps(vector<Match>&);
}
