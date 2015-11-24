//
//  PerfTests.cpp
//  Fleece
//
//  Created by Jens Alfke on 11/21/15.
//  Copyright © 2015 Couchbase. All rights reserved.
//

#include "FleeceTests.hh"
#include "JSONReader.hh"
#include <assert.h>

#define kDir "/Couchbase/Fleece/Tests/"

using namespace fleece;

class PerfTests : public CppUnit::TestFixture {
    public:

    void testConvert1000People() {
        const int kSamples = 50;
        double total = 0, minTime = 1e99, maxTime = -1;
        alloc_slice input = readFile(kDir "1000people.json");

        fprintf(stderr, "Converting JSON to Fleece (ms):");
        for (int i = 0; i < kSamples; i++) {
            Writer writer(input.size);
            encoder e(writer);
            JSONReader jr(e);
            Stopwatch st;

            jr.writeJSON(input);
            e.end();
            auto result = writer.extractOutput();

            double elapsed = st.elapsedMS();
            fprintf(stderr, " %g", elapsed);
            total += elapsed;
            minTime = std::min(minTime, elapsed);
            maxTime = std::max(maxTime, elapsed);

            if (i == kSamples-1) {
                fprintf(stderr, "\nJSON size: %zu bytes; Fleece size: %zu bytes (%.2f%%)\n",
                        input.size, result.size, (result.size*100.0/input.size));
                writeToFile(result, "1000people.fleece");
            }
}
        fprintf(stderr, "Average time is %g ms\n", (total - minTime - maxTime)/(kSamples-2));
    }
    
    CPPUNIT_TEST_SUITE( PerfTests );
    CPPUNIT_TEST( testConvert1000People );
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PerfTests);
