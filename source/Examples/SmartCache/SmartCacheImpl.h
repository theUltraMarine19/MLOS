//*********************************************************************
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root
// for license information.
//
// @File: SmartCacheImpl.h
//
// Purpose:
//      <description>
//
// Notes:
//      <special-instructions>
//
//*********************************************************************

#pragma once

template<typename TKey, typename TValue>
class LFUSmartCacheImpl
{
private:
    // map of frequency to corresponding list of keys
    std::unordered_map<int, typename std::list<TKey>> m_KeySequence;
    // mapping of keys to their values and frequency of occurrence
    std::unordered_map<TKey, typename std::pair<TValue, int>> m_elementSequence;
    // mapping of keys to their position in frequency lists
    std::unordered_map<TKey, typename std::list<TValue>::iterator> m_lookupTable;

    int m_cacheSize;
    // minimum frequency value of keys so far
    int min_frequency;
    Mlos::Core::ComponentConfig<SmartCache::SmartCacheConfig>& m_config;

public:
    LFUSmartCacheImpl(Mlos::Core::ComponentConfig<SmartCache::SmartCacheConfig>& config);
    ~LFUSmartCacheImpl();

    bool Contains(TKey key);
    TValue* Get(TKey key);
    void Push(TKey key, const TValue value);
    // update the frequency count of the key
    void Update(TKey key);

    void Reconfigure();
};

template<typename TKey, typename TValue>
inline LFUSmartCacheImpl<TKey, TValue>::LFUSmartCacheImpl(Mlos::Core::ComponentConfig<SmartCache::SmartCacheConfig>& config)
  : m_config(config)
{
    m_cacheSize = 0;

    // Apply initial configuration.
    Reconfigure();
}

template<class K, class V>
LFUSmartCacheImpl<K, V>::~LFUSmartCacheImpl()
{
}

template<typename TKey, typename TValue>
inline bool LFUSmartCacheImpl<TKey, TValue>::Contains(TKey key)
{
    bool isInCache = m_lookupTable.find(key) != m_lookupTable.end();

    SmartCache::CacheRequestEventMessage msg;
    msg.ConfigId = m_config.ConfigId;
    msg.Key = key;
    msg.IsInCache = isInCache;

    m_config.SendTelemetryMessage(msg);

    return isInCache;
}

template<typename TKey, typename TValue>
inline TValue* LFUSmartCacheImpl<TKey, TValue>::Get(TKey key)
{
    if (!Contains(key))
    {
        return nullptr;
    }

    Update(key);
    return &m_elementSequence[key].first;   
}

template<typename TKey, typename TValue>
inline void LFUSmartCacheImpl<TKey, TValue>::Push(TKey key, const TValue value)
{
    // Find the element ref in the lookup table.
    auto lookupItr = m_lookupTable.find(key);

    // element is not in Cache
    if (lookupItr == m_lookupTable.end())
    {
        if (m_lookupTable.size() == m_cacheSize)
        {
            // We reached the maximum cache size, evict the element based on the current policy.
            TKey evict_key = m_KeySequence[min_frequency].back();
            m_KeySequence[min_frequency].pop_back();
            m_lookupTable.erase(evict_key);
            m_elementSequence.erase(evict_key);
            
        }

        min_frequency = 1;
        m_KeySequence[min_frequency].push_front(key);
        m_elementSequence[key] = {value, min_frequency};
        m_lookupTable[key] = m_KeySequence[min_frequency].begin();
    }
    else
    {
        Update(key);
        m_elementSequence[key].first = value;
    }

    return;
}

template<typename TKey, typename TValue>
inline void LFUSmartCacheImpl<TKey, TValue>::Update(TKey key)
{
    // Lookup key
    auto lookupItr = m_lookupTable[key];
    int frequency = m_elementSequence[key].second;
    
    // Move key to next higher frequency list
    m_KeySequence[frequency].erase(lookupItr);
    m_KeySequence[frequency+1].push_front(key);
    m_elementSequence[key].second = frequency+1;
    m_lookupTable[key] = m_KeySequence[frequency+1].begin();
    
    // Update the minimum frequency value
    if (frequency == min_frequency && m_KeySequence[frequency].size() == 0)
        min_frequency++;
}

template<typename TKey, typename TValue>
inline void LFUSmartCacheImpl<TKey, TValue>::Reconfigure()
{
    // Update the cache size from the latest configuration available in shared memory.
    //
    m_cacheSize = m_config.CacheSize;

    // Clear the cache.
    //
    m_elementSequence.clear();
    m_KeySequence.clear();
    m_lookupTable.clear();

    // Adjust the number of buckets reserved for the cache (relative to the
    // max_load_factor) to match the new size.
    //
    m_lookupTable.reserve(m_cacheSize);
    m_elementSequence.reserve(m_cacheSize);
}

template<typename TKey, typename TValue>
class SmartCacheImpl
{
private:
    int m_cacheSize;

    std::list<TValue> m_elementSequence;

    std::unordered_map<TKey, typename std::list<TValue>::iterator> m_lookupTable;

    // Mlos Tunable Component Config.
    //
    Mlos::Core::ComponentConfig<SmartCache::SmartCacheConfig>& m_config;

    LFUSmartCacheImpl<TKey, TValue> lfu = LFUSmartCacheImpl<TKey, TValue>(m_config);

public:
    SmartCacheImpl(Mlos::Core::ComponentConfig<SmartCache::SmartCacheConfig>& config);
    ~SmartCacheImpl();

    bool Contains(TKey key);
    TValue* Get(TKey key);
    void Push(TKey key, const TValue value);

    void Reconfigure();
};

template<typename TKey, typename TValue>
inline SmartCacheImpl<TKey, TValue>::SmartCacheImpl(Mlos::Core::ComponentConfig<SmartCache::SmartCacheConfig>& config)
  : m_config(config)
{
    m_cacheSize = 0;
    
    // Apply initial configuration.
    //
    Reconfigure();
}

template<class K, class V>
SmartCacheImpl<K, V>::~SmartCacheImpl()
{
}

template<typename TKey, typename TValue>
inline bool SmartCacheImpl<TKey, TValue>::Contains(TKey key)
{
    if (m_config.EvictionPolicy == SmartCache::CacheEvictionPolicy::LeastFrequentlyUsed) {
        return lfu.Contains(key);
    }

    bool isInCache = m_lookupTable.find(key) != m_lookupTable.end();

    SmartCache::CacheRequestEventMessage msg;
    msg.ConfigId = m_config.ConfigId;
    msg.Key = key;
    msg.IsInCache = isInCache;

    m_config.SendTelemetryMessage(msg);

    return isInCache;
}

template<typename TKey, typename TValue>
inline TValue* SmartCacheImpl<TKey, TValue>::Get(TKey key)
{
    if (m_config.EvictionPolicy == SmartCache::CacheEvictionPolicy::LeastFrequentlyUsed) {
        return lfu.Get(key);
    }

    if (!Contains(key))
    {
        return nullptr;
    }

    // Find the element ref in the lookup table.
    //
    auto lookupItr = m_lookupTable.find(key);

    // Move the element to the beginning of the queue.
    //
    m_elementSequence.emplace_front(*lookupItr->second);
    m_elementSequence.erase(lookupItr->second);

    // As we moved the element, we need to update the element ref.
    //
    lookupItr->second = m_elementSequence.begin();

    return &m_elementSequence.front();
}

template<typename TKey, typename TValue>
inline void SmartCacheImpl<TKey, TValue>::Push(TKey key, const TValue value)
{
    // Find the element ref in the lookup table.
    //
    if (m_config.EvictionPolicy == SmartCache::CacheEvictionPolicy::LeastFrequentlyUsed)
    {
        lfu.Push(key, value);
        return;
    }
    
    auto lookupItr = m_lookupTable.find(key);

    if (lookupItr == m_lookupTable.end())
    {
        if (m_elementSequence.size() == m_cacheSize)
        {
            // We reached the maximum cache size, evict the element based on the current policy.
            //
            if (m_config.EvictionPolicy == SmartCache::CacheEvictionPolicy::LeastRecentlyUsed)
            {
                auto evictedLookupItr = m_elementSequence.back();
                m_elementSequence.pop_back();
                m_lookupTable.erase(evictedLookupItr);
            }
            else if (m_config.EvictionPolicy == SmartCache::CacheEvictionPolicy::MostRecentlyUsed)
            {
                auto evictedLookupItr = m_elementSequence.front();
                m_elementSequence.pop_front();
                m_lookupTable.erase(evictedLookupItr);
            }
            else
            {
                // Unknown policy.
                //
                throw std::exception();
            }
        }

        m_elementSequence.emplace_front(value);
        auto elementItr = m_elementSequence.begin();

        m_lookupTable.emplace(key, elementItr);
    }
    else
    {
        // Enqueue new element to the beginning of the queue.
        //
        m_elementSequence.emplace_front(value);
        m_elementSequence.erase(lookupItr->second);

        // Update existing lookup.
        //
        lookupItr->second = m_elementSequence.begin();
    }

    return;
}

template<typename TKey, typename TValue>
inline void SmartCacheImpl<TKey, TValue>::Reconfigure()
{
    if (m_config.EvictionPolicy == SmartCache::CacheEvictionPolicy::LeastFrequentlyUsed) {
        lfu.Reconfigure();
        return;
    }

    // Update the cache size from the latest configuration available in shared memory.
    //
    m_cacheSize = m_config.CacheSize;

    // Clear the cache.
    //
    m_elementSequence.clear();
    m_lookupTable.clear();

    // Adjust the number of buckets reserved for the cache (relative to the
    // max_load_factor) to match the new size.
    //
    m_lookupTable.reserve(m_cacheSize);
}
