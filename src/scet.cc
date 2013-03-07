#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <omp.h>

#include "ahocorasick.h"

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
    cerr << "USAGE: scet [options] term-file < corpus\n\n"
        " -h         : show this help\n"
        " -p (int)   : number of processors to use (default 1)\n\n"
        "Output thresholds:\n"
        " -a         : ignore all thresholds and output all pairs (overrides -l and -m)\n"
        " -l (float) : negative log likelihood cutoff (default 10)\n"
        " -m (float) : log mutual information cutoff (default 0)\n";
    exit(1);
}

int main(int argc, char* argv[]) {
    double LIKELIHOOD_CUTOFF = 10;
    double MI_CUTOFF = 0;
    bool OUTPUT_ALL = false;
    omp_set_num_threads(1);

    int c;

    while ((c = getopt(argc, argv, "ahl:m:p:")) != -1) {
        switch (c) {
            case 'p': omp_set_num_threads(atoi(optarg)); break;
            case 'h': usage(); break;
            case 'l': LIKELIHOOD_CUTOFF = atof(optarg); break;
            case 'm': MI_CUTOFF = atof(optarg); break;
            case 'a': OUTPUT_ALL = true; break;
        }
    }

    if (optind != (argc - 1))
        usage();

    ifstream strm(argv[optind]);
    ahocorasick::Trie t;
    int id;
    string token, line, name;

    map<int,string> t_id2extid;
    map<string, int> t_extid2id;

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
    t.build();

    map<int,int> mentions;
    map<int, map<int, int> > comentions;
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

            vector<ahocorasick::Match> matches = t.search(line);
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
    if (!OUTPUT_ALL) {
        for (auto kv1 : comentions) {
            int e1 = kv1.first;
            for (auto kv2 : kv1.second) {
                int e2 = kv2.first;
                int nAB = kv2.second;

                int nA = mentions[e1];
                int nB = mentions[e2];
                double mi = log(nAB * N_PROCESSED / (1.0 * nA * nB));

                double k = nAB;
                double lambda = 1.0 * nA * nB / N_PROCESSED;
                double likelihood = k * (log(k)-log(lambda)-1) + 0.5 * log(2 * M_PI * k) + lambda;
                string& e1_id = t_id2extid[e1];
                string& e2_id = t_id2extid[e2];
                
                if ((likelihood > LIKELIHOOD_CUTOFF) && (mi > MI_CUTOFF)) {
                    printf("%s\t%s\t%d\t%d\t%d\t%0.4f\t%0.4f\n", e1_id.c_str(), e2_id.c_str(), nA, nB, nAB, mi, likelihood);
                }
            }
        }
    } else {
        for (int e1=0; e1<t_id2extid.size(); e1++) {
            string& e1_id = t_id2extid[e1];
            int nA = mentions[e1];

            for (int e2=(e1+1); e2<t_id2extid.size(); e2++) {
                string& e2_id = t_id2extid[e2];
                int nB = mentions[e2];

                int nAB = comentions[e1][e2];
                double mi, likelihood;

                if (nAB > 0) {
                    mi = log(nAB * N_PROCESSED / (1.0 * nA * nB));
                    double k = nAB;
                    double lambda = 1.0 * nA * nB / N_PROCESSED;
                    likelihood = k * (log(k)-log(lambda)-1) + 0.5 * log(2 * M_PI * k) + lambda;
                } else {
                    mi = -INFINITY;
                    likelihood = -INFINITY;
                }
                
                printf("%s\t%s\t%d\t%d\t%d\t%0.4f\t%0.4f\n", 
                        e1_id.c_str(), e2_id.c_str(), nA, nB, nAB, mi, likelihood);
            }
        }
    }
}
