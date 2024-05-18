// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_CORE_X_LITERAL_HASH_MAP_HPP
#define ARCHON_X_CORE_X_LITERAL_HASH_MAP_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/hash.hpp>


namespace archon::core {


/// \brief Hash map that can be constructed at compile time
///
/// This class implements a hash map that can be constructed at compile time. See \ref
/// core::make_literal_hash_map() and \ref core::make_rev_literal_hash_map() for the easiest
/// way to construct such maps.
///
/// Literal hash maps need to be initialized by an invocation of either \re init() or \ref
/// init_rev(). This is done atomatically when constructing the map using \ref
/// core::make_literal_hash_map() or \ref core::make_rev_literal_hash_map().
///
/// Values are looked  up using \ref find().
///
/// Construction of a literal hash map is a `constexpr` operation if move construction and
/// invocation of a hash function of the specified type (\p H) are `constexpr` operations.
///
template<class K, class V, class H, std::size_t N, std::size_t M> class LiteralHashMap {
public:
    using key_type       = K;
    using value_type     = V;
    using hash_func_type = H;

    static constexpr std::size_t num_entries = N;
    static constexpr std::size_t num_buckets = M;

    static_assert(std::is_nothrow_move_constructible_v<hash_func_type>);
    static_assert(std::is_nothrow_invocable_v<hash_func_type, key_type>);

    constexpr LiteralHashMap(hash_func_type hash_func = {}) noexcept;

    constexpr void init(const std::pair<K, V>(& assocs)[N]) noexcept;
    constexpr void init_rev(const std::pair<V, K>(& assocs)[N]) noexcept;

    bool find(key_type key, value_type& value) const noexcept;

private:
    struct Entry {
        key_type key;
        value_type value;
    };

    struct Bucket {
        std::size_t offset;
    };

    struct State : hash_func_type {
        Entry entries[num_entries];
        Bucket buckets[num_buckets];
    };

    State m_state;

    constexpr auto hash(key_type key) const noexcept -> std::size_t;
};


/// \{
///
/// \brief Easy construction of literal hash map.
///
/// These functions offer an easy way of constructing and initlaizing a literal has map.
///
/// \tparam M Number of buckets. If set to zero, the number of buckets will be set equal to
/// the number of entries (number of specified association entries).
///
template<std::size_t M = 0, class K, class V, std::size_t N>
constexpr auto make_literal_hash_map(const std::pair<K, V>(& assocs)[N]) noexcept;

template<std::size_t M = 0, class K, class V, std::size_t N, class H>
constexpr auto make_literal_hash_map(const std::pair<K, V>(& assocs)[N], H&& hash_func) noexcept;

template<std::size_t M = 0, class K, class V, std::size_t N>
constexpr auto make_rev_literal_hash_map(const std::pair<V, K>(& assocs)[N]) noexcept;

template<std::size_t M = 0, class K, class V, std::size_t N, class H>
constexpr auto make_rev_literal_hash_map(const std::pair<V, K>(& assocs)[N], H&& hash_func) noexcept;
/// \}








// Implementation


template<class K, class V, class H, std::size_t N, std::size_t M>
constexpr LiteralHashMap<K, V, H, N, M>::LiteralHashMap(hash_func_type hash_func) noexcept
    : m_state { hash_func, {}, {} }
{
}


template<class K, class V, class H, std::size_t N, std::size_t M>
constexpr void LiteralHashMap<K, V, H, N, M>::init(const std::pair<K, V>(& assocs)[N]) noexcept
{
    std::size_t offset = 0;
    for (std::size_t i = 0; i < num_buckets; ++i) {
        m_state.buckets[i] = { offset };
        for (const auto& assoc : assocs) {
            key_type key     = assoc.first;
            value_type value = assoc.second;
            std::size_t bucket_index = hash(key);
            if (ARCHON_LIKELY(bucket_index != i))
                continue;
            m_state.entries[offset] = { key, value };
            ++offset;
        }
    }
    ARCHON_ASSERT(offset == num_entries);
}


template<class K, class V, class H, std::size_t N, std::size_t M>
constexpr void LiteralHashMap<K, V, H, N, M>::init_rev(const std::pair<V, K>(& assocs)[N]) noexcept
{
    std::size_t offset = 0;
    for (std::size_t i = 0; i < num_buckets; ++i) {
        m_state.buckets[i] = { offset };
        for (const auto& assoc : assocs) {
            key_type key     = assoc.second;
            value_type value = assoc.first;
            std::size_t bucket_index = hash(key);
            if (ARCHON_LIKELY(bucket_index != i))
                continue;
            m_state.entries[offset] = { key, value };
            ++offset;
        }
    }
    ARCHON_ASSERT(offset == num_entries);
}


template<class K, class V, class H, std::size_t N, std::size_t M>
bool LiteralHashMap<K, V, H, N, M>::find(key_type key, value_type& value) const noexcept
{
    std::size_t bucket_index = hash(key);
    const Bucket& bucket = m_state.buckets[bucket_index];
    std::size_t begin = bucket.offset;
    std::size_t end = (num_buckets - bucket_index == 1 ? num_entries : m_state.buckets[bucket_index + 1].offset);
    for (std::size_t i = begin; i < end; ++i) {
        const Entry& entry = m_state.entries[i];
        if (ARCHON_UNLIKELY(entry.key == key)) {
            value = entry.value;
            return true;
        }
    }
    return false;
}


template<class K, class V, class H, std::size_t N, std::size_t M>
constexpr auto LiteralHashMap<K, V, H, N, M>::hash(key_type key) const noexcept -> std::size_t
{
    const hash_func_type& hash_func = m_state;
    return std::size_t(hash_func(key) % num_buckets);
}


template<std::size_t M, class K, class V, std::size_t N>
constexpr auto make_literal_hash_map(const std::pair<K, V>(& assocs)[N]) noexcept
{
    return make_literal_hash_map<M>(assocs, core::Hash<K>());
}


template<std::size_t M, class K, class V, std::size_t N, class H>
constexpr auto make_literal_hash_map(const std::pair<K, V>(& assocs)[N], H&& hash_func) noexcept
{
    constexpr std::size_t num_buckets = (M > 0 ? M : N);
    core::LiteralHashMap<K, V, H, N, num_buckets> map(std::move(hash_func));
    map.init(assocs);
    return map;
}


template<std::size_t M, class K, class V, std::size_t N>
constexpr auto make_rev_literal_hash_map(const std::pair<V, K>(& assocs)[N]) noexcept
{
    return make_rev_literal_hash_map<M>(assocs, core::Hash<K>());
}


template<std::size_t M, class K, class V, std::size_t N, class H>
constexpr auto make_rev_literal_hash_map(const std::pair<V, K>(& assocs)[N], H&& hash_func) noexcept
{
    constexpr std::size_t num_buckets = (M > 0 ? M : N);
    core::LiteralHashMap<K, V, H, N, num_buckets> map(std::move(hash_func));
    map.init_rev(assocs);
    return map;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_LITERAL_HASH_MAP_HPP
