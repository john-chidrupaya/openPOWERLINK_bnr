/**
********************************************************************************
\file   ProcessImage.h

\brief  Abstract Design of ProcessImage structures
*******************************************************************************/

/*------------------------------------------------------------------------------
Copyright (c) 2014, Kalycito Infotech Private Limited
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
------------------------------------------------------------------------------*/

#ifndef _PROCESSIMAGE_H_
#define _PROCESSIMAGE_H_

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <map>
#include <vector>
#include <string>
#include <sstream>

#include "user/processimage/Channel.h"
#include "user/processimage/Direction.h"

#include "common/QtApiGlobal.h"

/**
 * \brief Abstract Design of ProcessImage structures
 *
 */
class PLKQTAPI_EXPORT ProcessImage
{
public:
	ProcessImage();

	/**
	 * \param byteSize  Total size of the ProcessImage in bytes.
	 * \param channels  The list of channels.
	 */
	ProcessImage(const unsigned int byteSize,
			const std::map<std::string, Channel>& channels);

	virtual ~ProcessImage();

	/**
	 * \brief   Set process-image size in bytes.
	 *
	 *        Beware: It is not enforced, that this size corresponds to the
	 *                cumulated size of all Channels.
	 * \param byteSize  Size in bytes.
	 */
	void SetSize(unsigned int byteSize);

	/**
	 * \brief   Returns the total size of the ProcessImage in bytes
	 *
	 * \return unsigned int  ProcessImage size in bytes.
	 */
	unsigned int GetSize() const;

	/**
	 * \brief   Sets the ProcessImage data pointer from the stack after memory
	 *          is allocated.
	 *
	 * \param data  Data pointer from the stack
	 */
	void SetProcessImageDataPtr(unsigned char* data);

	/**
	 * \brief   Returns the ProcessImage data pointer.
	 *
	 * \return unsigned char  Data pointer.
	 */
	unsigned char* GetProcessImageDataPtr() const;

	/**
	 * \brief   Returns the iterator to the first element.
	 *
	 * \return std::map<std::string, Channel>::const_iterator  Iterator to the start of the map.
	 */
	std::map<std::string, Channel>::const_iterator cbegin() const;

	/**
	 * \brief   Returns the iterator to the last element.
	 *
	 * \return std::map<std::string, Channel>::const_iterator  Iterator to the end of the map.
	 */
	std::map<std::string, Channel>::const_iterator cend() const;

	/**
	 * \brief   Inserts a Channel into the list of channels.
	 *
	 * \param channel  The Channel object.
	 * \retval true  If it is added successfully.
	 *         false If it fails to add.
	 */
	bool AddChannel(const Channel& channel);

	/**
	 * \param name           The Channel name.
	 * \return const Channel The Channel with the given name.
	 */
	const Channel GetChannel(const std::string& name) const;

	/**
	 * \brief Returns the list of Channel which has the same byte offset.
	 *
	 * \param byteOffset                  The byte offset value.
	 * \return const std::vector<Channel> The list of Channel.
	 */
	const std::vector<Channel> GetChannelsByOffset(const unsigned int byteOffset) const;

	/**
	 * \brief Returns the data size in bits for the given byte offset and bitoffset.
	 *
	 * \param byteOffset The byte offset value.
	 * \param bitOffset  The bit offset value.
	 * \return const unsigned int  Data size in bits.
	 */
	const unsigned int GetChannelsBitSize(const unsigned int byteOffset,
							const unsigned int bitOffset) const;

	/**
	 * \brief Returns the list of Channel which belong to the given nodeId.
	 *
	 * \param nodeId                      Node id of the node.
	 * \return const std::vector<Channel> The requested list of Channel.
	 */
	const std::vector<Channel> GetChannelsByNodeId(const unsigned int nodeId) const;

	/**
	 * \brief Returns the raw value of the ProcessImage variable.
	 *        The returned datatype depends on the datatype of the channel
	 *
	 * \param channelName The Channel name
	 * \return T          The templated return value.
	 *                    Depends on the datatype of the Channel.
	 */
	template<class T>
	T GetValue(const std::string& channelName) const;

protected:
	unsigned int byteSize;
	std::map<std::string, Channel> channels;
	unsigned char* data;

private:
	bool virtual AddChannelInternal(const Channel& channel) = 0;
};

#endif // _PROCESSIMAGE_H_
