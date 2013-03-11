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

#include "ahocorasick.h"
#include "acronym.h"

using namespace std;

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
        " -p (int)   : number of processors to use (default 1)\n"
        "\nSynonym dictionaries (at least one is required):\n"
        " -i (path)  : a tab-delimited file containing terms to be searched case insensitively\n"
        " -s (path)  : a tab-delimited file containing terms to be searched case sensitively\n"
        "\nOutput thresholds:\n"
        " -a         : ignore thresholds and output all pairs with at least 1 co-occurrence\n"
        "              (overrides -l and -m)\n"
        " -l (float) : negative log likelihood cutoff (default 10)\n"
        " -m (float) : log mutual information cutoff (default 0)\n";
    exit(1);
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
    double LIKELIHOOD_CUTOFF = 10;
    double MI_CUTOFF = 0;
    string CI_PATH; 
    string CS_PATH;
    bool OUTPUT_ALL = false;
    omp_set_num_threads(1);

    int c;

    while ((c = getopt(argc, argv, "ahl:m:p:i:s:")) != -1) {
        switch (c) {
            case 'p': omp_set_num_threads(atoi(optarg)); break;
            case 'h': usage(); break;
            case 'l': LIKELIHOOD_CUTOFF = atof(optarg); break;
            case 'm': MI_CUTOFF = atof(optarg); break;
            case 'i': CI_PATH = string(optarg); break;
            case 's': CS_PATH = string(optarg); break;
            case 'a': OUTPUT_ALL = true; break;
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
        map<pair<int,int>, int> C;

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
                for (int e2 : ids)
                    if (e1 < e2)
                        C[pair<int,int>(e1,e2)]++;
            }
        }

        #pragma omp critical
        {
            cerr << N_PROCESSED << endl;
            for (auto kv : M)
                mentions[kv.first] += kv.second;
            for (auto kv : C)
                comentions[kv.first.first][kv.first.second] += kv.second;
        }
    }
    #pragma omp taskwait
    }

    cout << "Entity1\tEntity2\tMentions1\tMentions2\tComentions\tMutualInformation\tLikelihood" << endl;
    map<int, set<int> > related;
    map<int, map<int, float> > m_mi;

    for (auto kv1 : comentions) {
        int e1 = kv1.first;
        string& e1_id = t_id2extid[e1];
        int nA = mentions[e1];

        for (auto kv2 : kv1.second) {
            int e2 = kv2.first;
            string& e2_id = t_id2extid[e2];
            int nB = mentions[e2];
            int nAB = kv2.second;

            double mi = log(nAB) + log(N_PROCESSED) - log(nA) - log(nB);
            double k = nAB;
            double lambda = 1.0 * nA * nB / N_PROCESSED;
            double likelihood = k * (log(k)-log(lambda)-1) + 0.5 * log(2 * M_PI * k) + lambda;

            if (OUTPUT_ALL || ((likelihood > LIKELIHOOD_CUTOFF) && (mi > MI_CUTOFF))) {
                printf("%s\t%s\t%d\t%d\t%d\t%0.4f\t%0.4f\n", 
                        e1_id.c_str(), e2_id.c_str(), nA, nB, nAB, mi, likelihood);
            }
        }
    }
} 
