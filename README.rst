==================================================
Significantly Co-occurring Entities in Text (SCET)
==================================================

Words or terms ("entities") which co-occur in text more often than expected by chance may share some kind of conceptual relationship. For example, in the biomedical domain, two genes which appear in many abstracts together may be involved in the same biochemical processes or diseases. In news articles, if two companies or names of people co-occur, they might be involved in the same industry or profession. 

Co-occurrence analysis is far from new (see References, below), but SCET aims to make these sorts of analyses simple and quick to perform on the UNIX command line for medium-sized corpora. As an idea, I can scan the biomedical database MEDLINE (16GB of text) on my desktop for relationships among 200,000+ entities in less than an hour.

Building and installation
=========================

SCET has no external dependencies, except a C++ compiler and make. Simply run:

.. code-block::

    make
    sudo make install

If you wish to use clang or some other C++ compiler than g++, edit the appropriate line in the Makefile.

Basic Usage
===========

SCET requires two types of input data, 1) a synonym dictionary, and 2) a text corpus.

The format of the synonym dictionary is a tab-delimited file, where the first column is the term ID (an arbitrary string), and subsequent fields are synonyms for that term. For example, some gene symbols:

.. code-block::

    A1BG	A1BG    A1B     ABG     GAB     HYST2477
    A2M	A2M     A2MD    CPAMD5  FWP007  S863-7
    A2MP1	A2MP1   A2MP

The text corpus is simply a flat text file, where each line is a different document. You can therefore run the entire program like so:

.. code-block::

    scet -i synonym-file < corpus.txt

The ``-i`` argument indicates that the synonyms are to be searched case-insensitively. You can provide an additional synonym file to be searched case-sensitively with ``-s``. SCET also comes with an acronym expansion module, which can be enabled with ``-a``; these options can be useful if you have synonyms which can be ambiguous (for example, "BED", "AIR", "AN", and "T" are all official HGNC gene symbols!).

A full list of options can be viewed by running ``./scet`` without any arguments, or ``scet -h``.

Output Format
=============

When analysis is complete, a tab-delimited file will be output, showing different metrics about the occurrences and co-occurrences of significantly interacting entities. 

The following fields contain general information about each entity:

- **Entity1**: the ID of the first entity
- **Entity2**: the ID of the second entity
- **Mentions1**: the number of documents Entity1 occurred in
- **Mentions2**: the number of documents Entity2 occurred in

The following fields measure information about direct co-occurrences of the two entities:

- **Comentions**: the number of documents the entities co-occurred in
- **MutualInformation**: the (natural log) mutual information of the interaction between the entities. (see Wren references, below)
- **Likelihood**: the negative natural log of the likelihood ratio for this pair of entities, calculated via Poisson approximation (see Bordag reference, below)

The following fields measure "indirect" relationships; i.e., whether Entity1 and Entity2 share many relationships with common intermediate entities.

- **nShared**: the number of intermediate entities that Entity1 and Entity2 share
- **nSignificantShared**: the number of intermediate entities that Entity1 and Entity2 share, where both Entity1 and Entity2 have a likelihood ratio > 8 with the intermediate
- **Jaccard**: the Jaccard coefficient, calculated as the number of shared, significant entities, divided by the number of distinct significant entities related to both Entity1 and Entity2 (see Bordag)
- **meanMI**: the mean minimum mutual information (see Wren)

A Concrete Example
==================

.. code-block::

    curl ftp://ftp.ncbi.nlm.nih.gov/gene/DATA/gene_info.gz \ 
        | gzip -dc | awk 'BEGIN {OFS="\t"} $1==9606 {print $3,$3,$5}' \
        | sed 's/|/\t/g' | sed 's/\t-//g' > gene_symbols
    wget http://corygil.es/data/medline.sample.gz
    zcat medline.sample.gz | ./scet gene_symbols -i > result.txt

Auxiliary scripts
=================

Some scripts are included in the script/ folder that assist with using SCET on biomedical data.

- **medline-txt** converts NCBI MEDLINE XML into tab-delimited output.
- **obo-synonyms** converts an Open Biomedical Ontology format file to a synonym file (Note: requires the pyparsing library to be installed).

References
==========

- http://bioinformatics.oxfordjournals.org/content/20/3/389.long
- http://www.ncbi.nlm.nih.gov/pmc/articles/PMC526381/
- http://wortschatz.uni-leipzig.de/~sbordag/papers/Bordag_05.pdf

License
=======

MIT/X11.
