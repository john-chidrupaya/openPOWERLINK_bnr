/**
********************************************************************************
\file   ProcessImage.h

\brief  Abstract Design of the ProcessImage structure

\author Ramakrishnan Periyakaruppan

\copyright (c) 2014, Kalycito Infotech Private Limited
					 All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the copyright holders nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef _PROCESSIMAGE_H_
#define _PROCESSIMAGE_H_

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include <map>
#include <vector>
#include <string>

#include "user/processimage/Channel.h"
#include "user/processimage/Direction.h"

#include "common/QtApiGlobal.h"

/**
 * \brief ProcessImage is a pure abstract base class which provides the
 * methods common to all the ProcessImage channels.  // XXX John: How to use this??
 *
 */
class PLKQTAPI_EXPORT ProcessImage
{
public:
	ProcessImage();

	/**
	 * \param[in] byteSize  Total size of the ProcessImage in bytes.
	 * \param[in] channels  The list of channels.
	 */
	ProcessImage(const UINT byteSize,
			const std::map<std::string, Channel>& channels);

	virtual ~ProcessImage();

	/**
	 * \brief   Set process-image size in bytes.
	 *
	 * \warning It is not enforced, that this size corresponds to the
	 *           cumulated size of all Channels.
	 * \param[in] byteSize  Size in bytes.
	 */
	void SetSize(UINT byteSize);

	/**
	 * \brief   Returns the total size of the ProcessImage in bytes
	 *
	 * \return ProcessImage size in bytes.
	 */
	UINT GetSize() const;

	/**
	 * \brief   Sets the ProcessImage data pointer from the stack after memory
	 *          is allocated.
	 *
	 * \param[in] data  Data pointer from the stack
	 */
	void SetProcessImageDataPtr(const BYTE* data);

	/**
	 * \return The ProcessImage pointer to the data allocated in the stack.
	 */
	BYTE* GetProcessImageDataPtr() const;

	/**
	 * \return Returns the const Iterator to the first element of the map.
	 */
	std::map<std::string, Channel>::const_iterator cbegin() const;

	/**
	 * \return Returns the iterator to the last element of the map.
	 */
	std::map<std::string, Channel>::const_iterator cend() const;

	/**
	 * \brief   Inserts a Channel into the list of channels.
	 *
	 * \param[in] channel  The Channel object.
	 * \retval true  If it is added successfully.
	 * \retval false If it fails to add.
	 */
	bool AddChannel(const Channel& channel);

	/**
	 * \param[in] name  The name of the Channel.
	 * \return The Channel with the given name.
	 * \throws std::out_of_range If name not present in the map.
	 */
	const Channel GetChannel(const std::string& name) const;

	/**
	 * \brief Returns the list of Channel which has the same byte offset.
	 *
	 * \param[in] byteOffset  The byte offset value.
	 * \return The list of Channel.
	 */
	const std::vector<Channel> GetChannelsByOffset(const UINT byteOffset) const;

	/**
	 * \brief Returns the list of Channel which belong to the given nodeId.
	 *
	 * \param[in] nodeId  Node id of the node.
	 * \return The requested list of Channel.
	 */
	const std::vector<Channel> GetChannelsByNodeId(const UINT nodeId) const;

	/**
	 * \brief   Returns the value in 'Big Endian' that the Channel holds.
	 *
	 * \param[in] channelName  The Channel name.
	 * \return Returns the requested value
	 * \throws std::out_of_range If name not found
	 */
	std::vector<BYTE> GetRawValue(const std::string& channelName) const;

	/**
	 * \brief   Returns the value in 'Big Endian' that the Channel holds.
	 * \param[in] channelName The Channel name.
	 * \param[out] value      The requested value.
	 * \param[in] dataLen     Size of the data in bits.
	 */
	void GetRawValue(const std::string& channelName,
						void* const value,
						size_t dataLen) const;

	/**
	 * \brief   Returns the value 'Big Endian' present at the given BYTE and bit offsets.
	 *
	 * \param[in] bitSize     Size of the data in bits.
	 * \param[in] byteOffset  Offset in bytes.
	 * \param[in] bitOffset   Offset in bits with in a single BYTE
	 *						  (i.e. in the range of 0 to 7).
	 * \return Returns the requested value.
	 */
	std::vector<BYTE> GetRawData(const UINT bitSize,
									const UINT byteOffset,
									const UINT bitOffset = 0) const;

	/**
	 * \brief Returns the raw value of the ProcessImage variable.
	 *        The returned datatype depends on the datatype of the channel
	 *
	 * \param[in] channelName The Channel name
	 * \return T  The templated return value. Depends on the datatype of the Channel.
	 */
	template<class T>
	T GetValue(const std::string& channelName) const;

protected:
	UINT byteSize;
	std::map<std::string, Channel> channels;
	BYTE* data;  ///< Pointer to access the ProcessImage data allocated in the Stack.

private:
	bool virtual AddChannelInternal(const Channel& channel) = 0;
};

#endif // _PROCESSIMAGE_H_
