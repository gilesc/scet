#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <omp.h>
#include <algorithm>

#include "ahocorasick.h"
#include "acronym.h"

using namespace std;

enum Mode {
    MENTION, COMENTION, IMPLICIT
};

vector<string> get_chunk(istream& is) {
    vector<string> lines;
    string line;
    int n = 0;
    while (getline(is, line) && (n++ < 20000)) {
        lines.push_back(line);
    }
    return lines;
}

void usage() {
    cerr << "USAGE: scet [options] < corpus\n"
        "\nGeneral:\n"
        " -h         : show this help\n"
        " -v         : verbose mode\n"
        " -p (int)   : number of processors to use (default 1)\n"
        " -a         : output all entity pairs,\n" 
        "              whether they are comentioned or not\n"
        " -m         : output mode. one of: [mention, comention, implicit]\n"
        "              (default implicit)\n"
        "\nSynonym dictionaries (at least one is required):\n"
        " -i (path)  : a tab-delimited file containing terms to be searched case insensitively\n"
        " -s (path)  : a tab-delimited file containing terms to be searched case sensitively\n";
    exit(1);
}

template <class T>
pair<int,double> jaccard_coefficient (const set<T>& A, const set<T>& B) {
    set<T> tmp;
    set_intersection(A.begin(),A.end(),
            B.begin(),B.end(),
            inserter(tmp,tmp.end()));
    int _intersection = tmp.size();
    tmp.clear();
    set_union(A.begin(),A.end(),
            B.begin(),B.end(),
            inserter(tmp,tmp.end()));
    int _union = tmp.size();
    if (!_union)
        return pair<int,double>(0,0);
    return pair<int, double>(_intersection, 1.0 * _intersection / _union);
}

void 
fill_trie(ahocorasick::Trie& t, string path, 
        map<int,string>& t_id2extid, map<string, int>& t_extid2id) {
    // Populate a Trie from synonym file and simultaneously populates ID maps
    int id;
    string token, line, name;

        if (!path.empty()) {
        ifstream strm(path.c_str());

        while (getline(strm, line)) {
            istringstream ss(line);
            string term, ext_id;
            getline(ss, ext_id, '\t');

            if (t_extid2id.find(ext_id) == t_extid2id.end()) {
                id = t_id2extid.size(); 
                t_id2extid[id] = ext_id;
                t_extid2id[ext_id] = id;
            } else {
                id = t_extid2id[ext_id];
            }

            while (getline(ss, term, '\t'))
                t.add(id, term);
        }
    }
    t.build();
}

int main(int argc, char* argv[]) {
    bool VERBOSE = false;
    bool OUTPUT_ALL = false;
    Mode MODE = IMPLICIT;
    string CI_PATH; 
    string CS_PATH;
    omp_set_num_threads(1);

    int c;

    while ((c = getopt(argc, argv, "vhap:i:s:m:")) != -1) {
        switch (c) {
            case 'p': omp_set_num_threads(atoi(optarg)); break;
            case 'h': usage(); break;
            case 'i': CI_PATH = string(optarg); break;
            case 's': CS_PATH = string(optarg); break;
            case 'a': OUTPUT_ALL = true;
            case 'm': 
                      if (!strcmp(optarg, "mention"))
                          MODE = MENTION;
                      else if (!strcmp(optarg,"comention"))
                          MODE = COMENTION;
                      else if (!strcmp(optarg, "implicit"))
                          MODE = IMPLICIT;
                      else {
                          cerr << "ERROR: mode, if specified, must be one of: 'mention', 'comention', or 'implicit'! Aborting.\n";
                          exit(1);
                      }
                      break;
            case 'v': VERBOSE = true; break;
        }
    }

    if ((optind != argc) || (CI_PATH.empty() && CS_PATH.empty()))
        usage();

    map<int,string> t_id2extid;
    map<string, int> t_extid2id;
    ahocorasick::Settings settings_ci;
    settings_ci.case_sensitive = false;
    ahocorasick::Trie ci(settings_ci);
    ahocorasick::Settings settings_cs;
    settings_cs.case_sensitive = true;
    ahocorasick::Trie cs(settings_cs);

    fill_trie(ci, CI_PATH, t_id2extid, t_extid2id);
    fill_trie(cs, CS_PATH, t_id2extid, t_extid2id);

    map<int,int> mentions;
    map<int, map<int, int> > comentions;
    map<int,int> mentions_bd;
    map<int, map<int, int> > comentions_bd;
    string line;
    vector<string> lines;
    int N_PROCESSED = 0;

    #pragma omp parallel shared (mentions, comentions, N_PROCESSED) private (line, lines)
    {
    #pragma omp master
    while (!(lines = get_chunk(cin)).empty())
    #pragma omp task
    {
        map<int,int> M;
        map<int, map<int, int>> C;
        map<int,int> Bm;
        map<int, map<int, int>> Bc;


        for (string line : lines) {
            #pragma omp atomic
            N_PROCESSED++;

            replace_acronyms_with_long_forms(line);

            vector<ahocorasick::Match> matches = ci.search(line);
            vector<ahocorasick::Match> cs_matches = cs.search(line);
            matches.insert(matches.end(), cs_matches.begin(), cs_matches.end());
            ahocorasick::remove_overlaps(matches);
            set<int> ids;
            for (ahocorasick::Match match : matches) 
                ids.insert(match.id);

            for (int e1 : ids) {
                M[e1]++;
                if (Bm.find(e1)==Bm.end()) Bm[e1] = N_PROCESSED+1;
                for (int e2 : ids)
                    if (e1 < e2) {
                        C[e1][e2]++;
                        if (Bc[e1].find(e2)==Bc[e1].end()) 
                            Bc[e1][e2] = N_PROCESSED+1;
                    }
            }
        }

        #pragma omp critical
        {
            if (VERBOSE)
                cerr << N_PROCESSED << endl;
            for (auto kv : M)
                mentions[kv.first] += kv.second;
            for (auto kv1 : C)
                for (auto kv2 : kv1.second)
                    comentions[kv1.first][kv2.first] += kv2.second;
            for (auto kv : Bm)
                if (!mentions_bd[kv.first])
                   mentions_bd[kv.first] = kv.second;
            for (auto kv1 : Bc)
                for (auto kv2 : kv1.second)
                    if (!comentions_bd[kv1.first][kv2.first])
                        comentions_bd[kv1.first][kv2.first] = kv2.second;
        }
    }
    #pragma omp taskwait
    }

    if (MODE == MENTION) {
        printf("Entity\tFirstObserved\tMentions\n");
        for (auto kv : mentions) {
            int id = kv.first;
            printf("%s\t%d\t%d\n", t_id2extid[id].c_str(), 
                    mentions_bd[id], kv.second);
        }
    } else { 
    if (MODE == COMENTION)
        cout << "Entity1\tEntity2\tFirstObserved\tComentions\n";
    else if (MODE == IMPLICIT)
        cout << "Entity1\tEntity2\tMentions1\tMentions2\tComentions\tMutualInformation\tLikelihood\tnShared\tnSignificantShared\tJaccard\tmeanMI\n";
    
    map<int, set<int> > related;
    map<int, map<int, float> > m_mi;

    for (auto kv1 : comentions) {
        int e1 = kv1.first;
        int nA = mentions[e1];

        for (auto kv2 : kv1.second) {
            int e2 = kv2.first;
            int nB = mentions[e2];
            int nAB = kv2.second;
            int birth_date = comentions_bd[e1][e2];
            if (MODE == COMENTION)
                printf("%s\t%s\t%d\t%d\n", t_id2extid[e1].c_str(), 
                        t_id2extid[e2].c_str(), birth_date, nAB);

            double mi = log(nAB) + log(N_PROCESSED) - log(nA) - log(nB);
            double k = nAB;
            double lambda = 1.0 * nA * nB / N_PROCESSED;
            double likelihood = k * (log(k)-log(lambda)-1) + 0.5 * log(2 * M_PI * k) + lambda;

            m_mi[e1][e2] = mi;

            if (likelihood > 8) {
                related[e1].insert(e2);
                related[e2].insert(e1);
            }
        }
    }

    for (auto kv1 : related) {
        int e1 = kv1.first;
        int nA = mentions[e1];
        map<int, int>& _comentions = comentions[e1];
        map<int, float>& _mi1 = m_mi[e1];
        string& e1_id = t_id2extid[e1];

        for (auto kv2 : related) {
            int e2 = kv2.first;
            if (e2 < e1)
                continue;
            string& e2_id = t_id2extid[e2];

            int nB = mentions[e2];
            int nAB = _comentions.find(e2)!=_comentions.end() ? comentions[e1][e2] : 0;

            double mi, likelihood;
            if (nAB) {
                mi = log(nAB) + log(N_PROCESSED) - log(nA) - log(nB);
                int k = nAB;
                double lambda = 1.0 * nA * nB / N_PROCESSED;
                likelihood = k * (log(k)-log(lambda)-1) + 0.5 * log(2 * M_PI * k) + lambda;
            } else {
                if (!OUTPUT_ALL)
                    continue;
                mi = -INFINITY;
                likelihood = -INFINITY;
            }
            pair<int,double> jaccard = jaccard_coefficient(kv1.second, kv2.second);

            map<int, float>& _mi2 = m_mi[e2];
            double mmim = 0;
            int n_shared = 0;
            for (auto Bpr : _mi1)
                if (_mi2.find(Bpr.first) != _mi2.end()) {
                    mmim = min(Bpr.second, _mi2[Bpr.first]);
                    n_shared++;
                }
            if (mmim)
                mmim /= n_shared;

            printf("%s\t%s\t%d\t%d\t%d\t%0.4f\t%0.4f\t%d\t%d\t%0.4f\t%0.4f\n", 
                    e1_id.c_str(), e2_id.c_str(), nA, nB, nAB, mi, likelihood, 
                    n_shared, jaccard.first, jaccard.second, mmim);
        }
    }
    }
} 
