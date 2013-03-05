==================================================
Significantly Co-occurring Entities in Text (SCET)
==================================================

Words or terms ("entities") which co-occur in text more often than expected by chance may share some kind of conceptual relationship. For example, in the biomedical domain, two genes which appear in many abstracts together may be involved in the same biochemical processes or diseases. In news articles, if two companies or names of people co-occur, they might be involved in the same industry or profession. 

Co-occurrence analysis is far from new (see References, below), but SCET aims to make these sorts of analyses simple and quick to perform on the UNIX command line for medium-sized corpora. As an idea, I can scan the biomedical database MEDLINE (16GB of text) on my desktop for relationships among 200,000+ entities in less than an hour.

Building and installation
=========================

SCET has no external dependencies, except a C++ compiler. Simply run:

    make
    sudo make install

If you wish to use clang or some other C++ compiler than g++, edit the appropriate line in the Makefile.

Basic Usage
===========

SCET requires two types of input data, 1) a synonym dictionary, and 2) a text corpus.

The format of the synonym dictionary is a tab-delimited file, where the first column is the term ID (an arbitrary string), and subsequent fields are synonyms for that term. For example, some gene symbols:

    A1BG    A1B     ABG     GAB     HYST2477
    A2M     A2MD    CPAMD5  FWP007  S863-7
    A2MP1   A2MP

The text corpus is simply a flat text file, where each line is a different document. You can therefore run the entire program like so:

    scet synonym-file < corpus.txt

Output Format
=============

When analysis is complete, a 3-column file will be output. The first two columns are term IDs for significantly-cooccurring entities. The third column is a double indicating how significant the interaction is: the higher, the more significant. Note that by default, only significantly co-occurring entities will be output.

(TODO: give more specifics about derivation.) 

A Concrete Example
==================

    curl ftp://ftp.ncbi.nlm.nih.gov/gene/DATA/gene_info.gz \ 
        | gzip -dc | awk 'BEGIN {OFS="\t"} $1==9606 {print $3,$5}' \
        | sed 's/|/\t/g' > gene_symbols
    wget http://corygil.es/data/medline.sample.gz
    zcat medline.sample.gz | ./scet gene_symbols > result.txt

medline2txt
===========

Also included is a Python script, medline2txt, that will convert MEDLINE XML into a 3-column text format: PMID, title, and abstract. To get the full MEDLINE database, you have to have a license from NCBI.

References
==========

* http://bioinformatics.oxfordjournals.org/content/20/3/389.long
* http://www.ncbi.nlm.nih.gov/pmc/articles/PMC526381/
* http://wortschatz.uni-leipzig.de/~sbordag/papers/Bordag_05.pdf

License
=======

MIT/X11.