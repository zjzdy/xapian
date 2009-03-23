/** @file api_percentages.cc
 * @brief Tests of percentage calculations.
 */
/* Copyright 2008,2009 Lemur Consulting Ltd
 * Copyright 2008,2009 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include "api_percentages.h"

#include <xapian.h>

#include "apitest.h"
#include "testutils.h"

#include <cfloat>

using namespace std;

// Test that percentages reported are the same regardless of which part of the
// mset is returned, for sort-by-value search.  Regression test for bug#216 in
// 1.0.10 and earlier with returned percentages.
DEFINE_TESTCASE(consistency3, backend) {
    Xapian::Database db(get_database("apitest_sortconsist"));
    Xapian::Enquire enquire(db);
    enquire.set_query(Xapian::Query("foo"));
    enquire.set_sort_by_value(1, 0);
    Xapian::doccount lots = 3;
    Xapian::MSet bigmset = enquire.get_mset(0, lots);
    TEST_EQUAL(bigmset.size(), lots);
    for (Xapian::doccount start = 0; start < lots; ++start) {
	tout << *bigmset[start] << ":" << bigmset[start].get_weight() << ":"
	     << bigmset[start].get_percent() << "%" << endl;
	for (Xapian::doccount size = 0; size < lots - start; ++size) {
	    Xapian::MSet mset = enquire.get_mset(start, size);
	    if (mset.size()) {
		TEST_EQUAL(start + mset.size(),
			   min(start + size, bigmset.size()));
	    } else if (size) {
		TEST(start >= bigmset.size());
	    }
	    for (Xapian::doccount i = 0; i < mset.size(); ++i) {
		TEST_EQUAL(*mset[i], *bigmset[start + i]);
		TEST_EQUAL_DOUBLE(mset[i].get_weight(),
				  bigmset[start + i].get_weight());
		TEST_EQUAL_DOUBLE(mset[i].get_percent(),
				  bigmset[start + i].get_percent());
	    }
	}
    }
    return true;
}

class MyPostingSource : public Xapian::PostingSource {
    std::vector<std::pair<Xapian::docid, Xapian::weight> > weights;
    std::vector<std::pair<Xapian::docid, Xapian::weight> >::const_iterator i;
    Xapian::weight maxwt;
    bool started;

    MyPostingSource(const std::vector<std::pair<Xapian::docid, Xapian::weight> > &weights_,
		    Xapian::weight maxwt_)
	: weights(weights_), maxwt(maxwt_), started(false)
    {}

  public:
    MyPostingSource() : maxwt(0.0), started(false) { }

    PostingSource * clone() const
    {
	return new MyPostingSource(weights, maxwt);
    }

    void append_docweight(Xapian::docid did, Xapian::weight wt) {
	weights.push_back(make_pair(did, wt));
	if (wt > maxwt) maxwt = wt;
    }

    void set_maxweight(Xapian::weight wt) {
	if (wt > maxwt) maxwt = wt;
    }

    void init(const Xapian::Database &) { started = false; }

    Xapian::weight get_weight() const { return i->second; }

    Xapian::weight get_maxweight() const { return maxwt; }

    Xapian::doccount get_termfreq_min() const { return weights.size(); }
    Xapian::doccount get_termfreq_est() const { return weights.size(); }
    Xapian::doccount get_termfreq_max() const { return weights.size(); }

    void next(Xapian::weight /*wt*/) {
	if (!started) {
	    i = weights.begin();
	    started = true;
	} else {
	    ++i;
	}
    }

    bool at_end() const {
	return (i == weights.end());
    }

    Xapian::docid get_docid() const { return i->first; }

    std::string get_description() const {
	return "MyPostingSource";
    }
};


/// Test for rounding errors in percentage weight calculations and cutoffs.
DEFINE_TESTCASE(pctcutoff4, backend && !remote && !multi) {
    // Find the number of DBL_EPSILONs to subtract which result in the
    // percentage of the second hit being 49% instead of 50%.
    int epsilons = 0;
    Xapian::Database db(get_database("apitest_simpledata"));
    Xapian::Enquire enquire(db);
    while (true) {
	MyPostingSource source;
	source.append_docweight(1, 100);
	source.append_docweight(2, 50 - epsilons * DBL_EPSILON);
	enquire.set_query(Xapian::Query(&source));
	Xapian::MSet mset = enquire.get_mset(0, 10);
	TEST_EQUAL(mset.size(), 2);
	if (mset[1].get_percent() != 50) break;
	++epsilons;
    }

    // Make a set of document weights including ones on either side of the
    // 49% / 50% boundary.
    MyPostingSource source;
    source.append_docweight(1, 100);
    source.append_docweight(2, 50);
    source.append_docweight(3, 50 - (epsilons - 1) * DBL_EPSILON);
    source.append_docweight(4, 50 - epsilons * DBL_EPSILON);
    source.append_docweight(5, 25);

    enquire.set_query(Xapian::Query(&source));
    Xapian::MSet mset1 = enquire.get_mset(0, 10);
    TEST_EQUAL(mset1.size(), 5);
    TEST_EQUAL(mset1[2].get_percent(), 50);
    TEST_EQUAL(mset1[3].get_percent(), 49);

    // Use various different percentage cutoffs, and check that the values
    // returned are as expected.
    int percent = 100;
    for (Xapian::MSetIterator i = mset1.begin(); i != mset1.end(); ++i) {
	int new_percent = mset1.convert_to_percent(i);
	tout << "mset1 item = " << i.get_percent() << "%\n";
	if (new_percent != percent) {
	    enquire.set_cutoff(percent);
	    Xapian::MSet mset2 = enquire.get_mset(0, 10);
	    tout << "cutoff = " << percent << "%, "
		    "mset size = " << mset2.size() << "\n";
	    TEST_EQUAL(mset2.size(), i.get_rank());
	    percent = new_percent;
	}
    }

    return true;
}

/// Check we throw for a percentage cutoff while sorting primarily by value.
DEFINE_TESTCASE(pctcutoff5, backend) {
    Xapian::Database db(get_database("apitest_simpledata"));
    Xapian::Enquire enquire(db);
    enquire.set_query(Xapian::Query("test"));
    enquire.set_cutoff(42);
    Xapian::MSet mset;

    enquire.set_sort_by_value(0, false);
    TEST_EXCEPTION(Xapian::UnimplementedError, mset = enquire.get_mset(0, 10));

    enquire.set_sort_by_value(0, true);
    TEST_EXCEPTION(Xapian::UnimplementedError, mset = enquire.get_mset(0, 10));

    enquire.set_sort_by_value_then_relevance(0, false);
    TEST_EXCEPTION(Xapian::UnimplementedError, mset = enquire.get_mset(0, 10));

    enquire.set_sort_by_value_then_relevance(0, true);
    TEST_EXCEPTION(Xapian::UnimplementedError, mset = enquire.get_mset(0, 10));

    return true;
}
