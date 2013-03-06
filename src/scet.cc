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
    while ((n++ < 10000) && getline(is, line)) {
        lines.push_back(line);
    }
    return lines;
}

void usage() {
    cerr << "USAGE: scet [options] term-file < corpus\n\n"
        " -h         : show this help\n"
        " -p (int)   : number of processors to use (default 1)\n"
        " -l (float) : negative log likelihood cutoff (default 10)\n"
        " -m (float) : log mutual information cutoff (default 0)\n";
    exit(1);
}

int main(int argc, char* argv[]) {
    double LIKELIHOOD_CUTOFF = 10;
    double MI_CUTOFF = 0;

    int c;

    while ((c = getopt(argc, argv, "p:")) != -1) {
        switch (c) {
            case 'p': omp_set_num_threads(atoi(optarg)); break;
            case 'h': usage(); break;
            case 'l': LIKELIHOOD_CUTOFF = atof(optarg); break;
            case 'm': MI_CUTOFF = atof(optarg); break;
        }
    }

    if (optind != (argc - 1))
        usage();

    ifstream strm(argv[1]);
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
    map<pair<int,int>, int> comentions;
    vector<string> lines;
    int N_PROCESSED = 0;

    #pragma omp parallel private (line, lines) shared (mentions, comentions)
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
            for (ahocorasick::Match m1 : matches) {
                M[m1.id]++;
                for (ahocorasick::Match m2 : matches)
                    if (m1.id < m2.id)
                        C[pair<int,int>(m1.id, m2.id)]++;
            }
        }

        #pragma omp critical
        {
            for (auto kv : M)
                mentions[kv.first] += kv.second;
            for (auto kv : C)
                comentions[kv.first] += kv.second;
        }
        lines.clear();
    }
    }

    cerr << "Entity1\tEntity2\tMentions1\tMentions2\tComentions\tMutualInformation\tLikelihood" << endl;
    for (auto kv : comentions) {
        int e1 = kv.first.first;
        int e2 = kv.first.second;
        int nAB = kv.second;
        int nA = mentions[e1];
        int nB = mentions[e2];
        double mi = log(nAB * N_PROCESSED / (nA * nB));

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
