//*********************************************************************
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root
// for license information.
//
// @File: Workloads.cpp
//
// Purpose:
//      <description>
//
// Notes:
//      <special-instructions>
//
//*********************************************************************

#include <stdlib.h>
#include "stdafx.h"
#include "SmartCacheImpl.h"
#include "Workloads.h"

uint64_t CyclicalWorkload(uint64_t sequenceNumber, SmartCacheImpl<int32_t, int32_t>& smartCache)
{
    for (int32_t i = 1; i < sequenceNumber; i++)
    {
        int32_t* element = smartCache.Get(i);
        if (element == nullptr)
        {
            smartCache.Push(i, i);
        }
    }

    return 1;
}

uint64_t RandomWorkload(uint64_t sequenceNumber, SmartCacheImpl<int32_t, int32_t>& smartCache)
{
	for (int32_t j = 0; j < 4; j++)
	{
	    for (int32_t i = 1; i < sequenceNumber/3; i++)
	    {
	        int32_t* element = smartCache.Get(i);
	        if (element == nullptr)
	        {
	            smartCache.Push(i, i);
	        }
	    }
	}
    for (int32_t i = sequenceNumber/3; i < 2*sequenceNumber/3; i++)
    {
        int32_t* element = smartCache.Get(i);
        if (element == nullptr)
        {
            smartCache.Push(i, i);
        }
        element = smartCache.Get(i - sequenceNumber/3);
        if (element == nullptr)
        {
            smartCache.Push(i - sequenceNumber/3, i - sequenceNumber/3);
        }
    }
    for (int32_t i = 1; i < sequenceNumber/3; i++)
    {
        int32_t* element = smartCache.Get(i);
        if (element == nullptr)
        {
            smartCache.Push(i, i);
        }
    }

    return 1;
}
