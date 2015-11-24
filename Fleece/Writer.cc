//
//  Writer.cc
//  Fleece
//
//  Created by Jens Alfke on 2/4/15.
//  Copyright (c) 2015 Couchbase. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//  Unless required by applicable law or agreed to in writing, software distributed under the
//  License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
//  either express or implied. See the License for the specific language governing permissions
//  and limitations under the License.

#include "Writer.hh"
#include <assert.h>


namespace fleece {

    Writer::Writer(size_t initialCapacity)
    :_chunkSize(initialCapacity),
     _length(0)
    {
        addChunk(initialCapacity);
    }

    Writer::Writer(Writer&& w)
    :_chunks(w._chunks)
    {
        w._chunks.resize(0);
    }

    Writer::~Writer() {
        for (auto chunk = _chunks.begin(); chunk != _chunks.end(); ++chunk)
            chunk->free();
    }

    Writer& Writer::operator= (Writer&& w) {
        _chunks = w._chunks;
        w._chunks.resize(0);
        return *this;
    }

    const void* Writer::curPos() const {
        return _chunks.back().available().buf;
    }

    size_t Writer::posToOffset(const void *pos) const {
        size_t offset = 0;
        for (auto chunk = _chunks.begin(); chunk != _chunks.end(); ++chunk) {
            if (chunk->contains(pos))
                return offset + chunk->offsetOf(pos);
            offset += chunk->length();
        }
        throw "invalid pos for posToOffset";
    }

    const void* Writer::write(const void* data, size_t length) {
        const void* result = _chunks.back().write(data, length);
        if (!result) {
            if (_chunkSize <= 64*1024)
                _chunkSize *= 2;
            addChunk(std::max(length, _chunkSize));
            result = _chunks.back().write(data, length);
            assert(result);
        }
        _length += length;
        return result;
    }

    void Writer::rewrite(const void *pos, slice data) {
        assert(pos); //FIX: Check that it's actually inside a chunk
        ::memcpy((void*)pos, data.buf, data.size);
    }

    void Writer::addChunk(size_t capacity) {
        auto chunk(capacity);
        _chunks.push_back(chunk);
    }

    alloc_slice Writer::extractOutput() {
        alloc_slice output;
        if (_chunks.size() == 1) {
            _chunks[0].resizeToFit();
            output = alloc_slice::adopt(_chunks[0].contents());
        } else {
            output = alloc_slice(length());
            void* dst = (void*)output.buf;
            for (auto chunk = _chunks.begin(); chunk != _chunks.end(); ++chunk) {
                auto contents = chunk->contents();
                memcpy(dst, contents.buf, contents.size);
                dst = offsetby(dst, contents.size);
                chunk->free();
            }
        }
        _chunks.resize(0);
        _length = 0;
        return output;
    }


#pragma mark - CHUNK:

    Writer::Chunk::Chunk()
    :_start(NULL)
    { }

    Writer::Chunk::Chunk(size_t capacity)
    :_start(::malloc(capacity)),
    _available(_start, capacity)
    {
        if (!_start)
            throw std::bad_alloc();
    }

    Writer::Chunk::Chunk(const Chunk& c)
    :_start(c._start),
     _available(c._available)
    { }

    void Writer::Chunk::free() {
        ::free(_start);
        _start = NULL;
    }

    const void* Writer::Chunk::write(const void* data, size_t length) {
        if (_available.size < length)
            return NULL;
        const void *result = _available.buf;
        if (data != NULL)
            ::memcpy((void*)result, data, length);
        _available.moveStart(length);
        return result;
    }

    void Writer::Chunk::resizeToFit() {
        size_t len = length();
        void *newStart = ::realloc(_start, len);
        if (!newStart)
            throw std::bad_alloc();
        _start = newStart;
        _available = slice(offsetby(newStart,len), (size_t)0);
    }

}