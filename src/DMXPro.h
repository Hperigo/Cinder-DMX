/*
 *  DMXPro.h
 *
 *  Created by Andrea Cuius
 *  The MIT License (MIT)
 *  Copyright (c) 2014 Nocte Studio Ltd.
 *
 *  www.nocte.co.uk
 *
 */

 ///
 /// View device specifications here: https://www.enttec.com/docs/dmx_usb_pro_api_spec.pdf
 /// Heavily modified from Andrea Cuius' implementation
 ///

#pragma once

#include "cinder/Thread.h"
#include "cinder/Serial.h"

const auto DMXPRO_START_MSG   = 0x7E;  // Start of message delimiter
const auto DMXPRO_END_MSG     = 0xE7;  // End of message delimiter
const auto DMXPRO_SEND_LABEL  = 6;     // Output Only Send DMX Packet Request
const auto DMXPRO_BAUD_RATE   = 57600; // virtual COM doesn't control the usb, this is just a dummy value
const auto DMXPRO_FRAME_RATE  = 35;    // dmx send frame rate
const auto DMXPRO_DATA_SIZE   = 513;   // include first byte 0x00, what's that?
const auto DMXPRO_PACKET_SIZE = 518;   // data + 4 bytes(DMXPRO_START_MSG, DMXPRO_SEND_LABEL, DATA_SIZE_LSB, DATA_SIZE_MSB) at the beginning and 1 byte(DMXPRO_END_MSG) at the end

// TODO: test this alarming claim:
//////////////////////////////////////////////////////////
// LAST 4 dmx channels seem not to be working, 508-511 !!!
//////////////////////////////////////////////////////////

/// Buffer to simplify building up data for DMX transmission.
class DMXColorBuffer {
public:
	DMXColorBuffer();
	/// Set an individual channel value.
	void setValue(uint8_t value, size_t channel);
	/// Set a color value across three channels.
	void setValue(const ci::Color8u &color, size_t channel);

	const uint8_t* data() const { return _data.data(); }
	size_t size() const { return _data.size(); }
private:
	std::array<uint8_t, 512> _data;
};

class DMXPro;
using DMXProRef = std::shared_ptr<DMXPro>;

class DMXPro{

public:

	static DMXProRef create( const std::string &deviceName )
	{
		return DMXProRef( new DMXPro( deviceName ) );
	}

	~DMXPro();

	void init(bool initWithZeros = true);

	static void listDevices()
	{
		const std::vector<ci::Serial::Device> &devices( ci::Serial::getDevices(true) );

		ci::app::console() << "--- DMX usb pro > List serial devices ---" << std::endl;

		for( std::vector<ci::Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt )
			ci::app::console() << deviceIt->getName() << std::endl;

		ci::app::console() << "-----------------------------------------" << std::endl;
	}

	static std::vector<std::string> getDevicesList()
	{
		std::vector<std::string> devicesList;

		const std::vector<ci::Serial::Device> &devices( ci::Serial::getDevices(true) );

		for( std::vector<ci::Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt )
			devicesList.push_back( deviceIt->getPath() );

		return devicesList;
	}

	void	setZeros();

	bool	isConnected() { return mSerial != NULL; }

	void	setValue(int value, int channel);
	void	bufferData(const std::vector<uint8_t> &data) {
		std::lock_guard<std::mutex> lock(mBodyMutex);
		mBody = data;
	}

	void	reconnect();

	void	shutdown(bool send_zeros = true);
	/// Buffer all message data to be sent on the next DMX update. Threadsafe.
	void bufferData(const std::vector<uint8_t> &data);
	void bufferData(const uint8_t *data, size_t size);
	void bufferData(const DMXColorBuffer &buffer) { bufferData(buffer.data(), buffer.size()); }
	/// Fill the data buffer with a single value. Threadsafe.
	void fillBuffer(uint8_t value);
	/// Set an individual channel value using 1-indexed positions.
	[[deprecated("This method can result in incomplete data being sent over the wire. Use bufferData with a DMXColorBuffer instead.")]]
	void	setValue(uint8_t value, int channel);

	std::string  getDeviceName() { return mSerialDeviceName; }

private:

	DMXPro( const std::string &deviceName );

	void initDMX();

	void initSerial(bool initWithZeros);

	void			sendDMXData();
	///
	void dataSendLoop();
	/// Actually send data to DMX device.
	void writeData();

private:

	unsigned char	*mDMXPacket;			// DMX packet, it contains bytes
	std::vector<uint8_t>	mBody;
	std::mutex						mBodyMutex;
	std::chrono::high_resolution_clock::duration mTargetFrameTime;
	ci::SerialRef	mSerial;				// serial interface
	int				mThreadSleepFor;		// sleep for N ms, this is based on the FRAME_RATE
	std::mutex      mDMXDataMutex;			// mutex unique lock
	std::string		mSerialDeviceName;		// usb serial device name
	std::thread      mSendDataThread;
	std::atomic_bool mRunSendDataThread;

private:
	// disallow
	DMXPro(const DMXPro&);
	DMXPro& operator=(const DMXPro&);

};
