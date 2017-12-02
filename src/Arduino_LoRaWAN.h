/* Arduino_LoRaWAN.h	Tue Oct 25 2016 00:10:57 tmm */

/*

Module:  Arduino_LoRaWAN.h

Function:
	The base class for arduino-lmic-based LoRaWAN nodes.

Version:
	V0.1.0	Tue Oct 25 2016 00:10:57 tmm	Edit level 1

Copyright notice:
	This file copyright (C) 2016 by

		MCCI Corporation
		3520 Krums Corners Road
                Ithaca, NY  14850

	An unpublished work.  All rights reserved.
	
	This file is proprietary information, and may not be disclosed or
	copied without the prior permission of MCCI Corporation.
 
Author:
	Terry Moore, MCCI Corporation	October 2016

Revision history:
   0.1.0  Tue Oct 25 2016 00:10:57  tmm
	Module created.

*/

#ifndef _ARDUINO_LORAWAN_H_		/* prevent multiple includes */
#define _ARDUINO_LORAWAN_H_

#include <stdint.h>
#include <Arduino.h>
#ifndef _MCCIADK_ENV_H_
# include <mcciadk_env.h>
#endif

class Arduino_LoRaWAN;

/*
|| You can use this for declaring event functions...
|| or use a lambda if you're bold; but remember, no
|| captures in that case.  Yes, we're *not* using one
|| of the several thousand C++ ways of doing this;
|| we're using C-compatible callbacks.
*/
MCCIADK_BEGIN_DECLS
typedef void ARDUINO_LORAWAN_EVENT_FN(void *, uint32_t);
MCCIADK_END_DECLS

class Arduino_LoRaWAN
        {
public:
	/*
	|| the internal LMIC wrapper
	*/
	class cLMIC; /* forward reference, see Arduino_LoRaWAN_lmic.h */

	/*
	|| debug things
	*/
	enum {
	     LOG_BASIC = 1 << 0,
	     LOG_ERRORS = 1 << 1,
	     LOG_VERBOSE = 1 << 2,
	     };

	// We must replicate the C structure from
	// "lmic.h" inside the class. Otherwise we'd
	// need to have all of lmic.h in scope everywhere,
	// which could cause naming clashes.
        struct lmic_pinmap {
                // Use this for any unused pins.
                static constexpr uint8_t LMIC_UNUSED_PIN = 0xff;
                static constexpr int NUM_DIO = 3;

                uint8_t nss;
                uint8_t rxtx;
                uint8_t rst;
                uint8_t dio[NUM_DIO];
                uint8_t rxtx_rx_active;
                uint32_t spi_freq;
                };


	enum class ProvisioningStyle
		{
		kNone, kABP, kOTAA
		};

	struct AbpProvisioningInfo
		{
		uint8_t		NwkSKey[16];
		uint8_t		AppSKey[16];
		uint32_t	DevAddr;
                uint32_t        NetID;
                uint32_t        FCntUp;
                uint32_t        FCntDown;
		};

	struct OtaaProvisioningInfo
		{
		uint8_t		AppKey[16];
		uint8_t		DevEUI[8];
		uint8_t		AppEUI[8];
		};

	struct ProvisioningInfo
		{
		ProvisioningStyle	Style;
		AbpProvisioningInfo     AbpInfo;
		OtaaProvisioningInfo    OtaaInfo;
		};

        struct ProvisioningTable
                {
                const ProvisioningInfo  *pInfo;
                unsigned                nInfo;
                };

	enum SessionInfoTag : uint8_t
		{
		kSessionInfoTag_V1 = 0x01,
		};

	struct SessionInfoHdr
		{
		uint8_t Tag;
		uint8_t Size;
		};

	// Version 1 of session info.
	struct SessionInfoV1
		{
		// to ensure packing, we just repeat the header.
		uint8_t		Tag;		// kSessionInfoTag_V1
		uint8_t		Size;		// sizeof(SessionInfoV1)
		uint8_t		Rsv2;		// reserved
		uint8_t		Rsv3;		// reserved
                uint32_t        NetID;          // the network ID
		uint32_t	DevAddr;	// device address
		uint8_t		NwkSKey[16];	// network session key
		uint8_t		AppSKey[16];	// app session key
                uint32_t        FCntUp;		// uplink frame count
                uint32_t        FCntDown;	// downlink frame count
		};

	// information about the curent session, stored persistenly if
	// possible. We allow for versioning, primarily so that (if we
	// choose) we can accommodate older versions and very simple
	// storage schemes. However, this is just future-proofing.
	union SessionInfo
		{
		// the header, same for all versions
		SessionInfoHdr	Hdr;
		// the body.
		SessionInfoV1	V1;
		};

        /*
        || the constructor.
        */
        Arduino_LoRaWAN();
        Arduino_LoRaWAN(const lmic_pinmap &pinmap) : m_lmic_pins(pinmap) {}

        /*
        || the begin function. Call this to start things (the constructor
        || does not!
        */
        bool begin(void);

	/*
	|| the function to call from your loop()
	*/
	void loop(void);

        /*
        || Reset the LMIC
        */
        void Reset(void);

        /*
        || Shutdown the LMIC
        */
        void Shutdown(void);

        /*
        || Registering listeners... returns true for
        || success.
        */
        bool RegisterListener(ARDUINO_LORAWAN_EVENT_FN *, void *);

	/*
	|| Dispatch an event to all listeners
	*/
	void DispatchEvent(uint32_t);

	uint32_t GetDebugMask() { return this->m_ulDebugMask; }
	uint32_t SetDebugMask(uint32_t ulNewMask)
		{
		const uint32_t ulOldMask = this->m_ulDebugMask;

		this->m_ulDebugMask = ulNewMask;

		return ulOldMask;
		}

	void LogPrintf(
		const char *fmt,
		...
		) __attribute__((__format__(__printf__, 2, 3)));
		/* format counts start with 2 for non-static C++ member fns */


	/*
	|| we only support a single instance, but we don't name it. During
	|| begin processing, we register, then we can find it.
	*/
	static Arduino_LoRaWAN *GetInstance()
		{
		return Arduino_LoRaWAN::pLoRaWAN;
		}

	bool GetTxReady();

        typedef void SendBufferCbFn(void *pCtx, bool fSuccess);

	bool SendBuffer(
		const uint8_t *pBuffer,
		size_t nBuffer,
                SendBufferCbFn *pDoneFn = nullptr,
                void *pCtx = nullptr,
		bool fConfirmed = false
		);

        typedef void ReceiveBufferCbFn(
		void *pCtx, 
		const uint8_t *pBuffer, 
		size_t nBuffer
		);

	void SetReceiveBufferBufferCb(
		ReceiveBufferCbFn *pReceiveBufferFn,
		void *pCtx = nullptr
		)
		{
		this->m_pReceiveBufferFn = pReceiveBufferFn;
		this->m_pReceiveBufferCtx = pCtx;
		}

	bool GetDevEUI(
		uint8_t *pBuf
		);

	bool GetAppEUI(
	    uint8_t *pBuf
	    );

	bool GetAppKey(
	    uint8_t *pBuf
	    );

        // the pins
        lmic_pinmap m_lmic_pins;

protected:
        // you must have a NetBegin() function or things won't work.
        virtual bool NetBegin(void) = 0;

        // you may have a NetJoin() function.
        // if not, the base function does nothing.
        virtual void NetJoin(void) 
                { /* NOTHING */ };

        // you may have a NetRxComplete() function. 
        // if not, the base function does nothing.
        virtual void NetRxComplete(void);

        // you may have a NetTxComplete() function. 
        // if not, the base function does nothing.
        virtual void NetTxComplete(void) 
                { /* NOTHING */ };

	// you should provide a function that returns the provisioning
	// style from stable storage; if you don't yet have provisioning
	// info, return ProvisioningStyle::kNone
	virtual ProvisioningStyle GetProvisioningStyle(void)
		{
		return ProvisioningStyle::kOTAA;
		}

	// you should provide a function that returns provisioning info from 
	// persistent storage. Called during initialization. If this returns
	// false, OTAA will be forced. If this returns true (as it should for
        // a saved session), 
	virtual bool GetAbpProvisioningInfo(
			AbpProvisioningInfo *pProvisioningInfo
			)
		{
		// if not provided, default zeros buf and returns false.
		if (pProvisioningInfo)
                    {
                    memset(
                        pProvisioningInfo,
                        0,
                        sizeof(*pProvisioningInfo)
                        );
                    }
		return false;
		}

	// you should provide a function that returns 
	// OTAA provisioning info from persistent storage. Only called
	// if you return ProvisioningStyle::kOtaa to GetProvisioningStyle().
	virtual bool GetOtaaProvisioningInfo(
			OtaaProvisioningInfo *pProvisioningInfo
			)
		{
		// if not provided, default zeros buf and returns false.
		if (pProvisioningInfo)
                    {
                    memset(
                        pProvisioningInfo,
                        0,
                        sizeof(*pProvisioningInfo)
                        );
                    }
		return false;
		}

	// if you have persistent storage, you should provide a function
	// that gets the saved session info from persistent storage, or
	// indicate that there isn't a valid saved session. Note that
	// the saved info is opaque to the higher level. The number of
	// bytes actually stored into pSessionInfo is returned. FCntUp
	// and FCntDown are stored separately.
	virtual bool GetSavedSessionInfo(
			SessionInfo *pSessionInfo,
			uint8_t *pExtraSessionInfo,
			size_t nExtraSessionInfo,
			size_t *pnExtraSessionActual
			)
		{
		// if not provided, default zeros buf and returns false.
		if (pExtraSessionInfo)
			{
			memset(pExtraSessionInfo, 0, nExtraSessionInfo);
			}
		if (pSessionInfo)
			{
			memset(pSessionInfo, 0, sizeof(*pSessionInfo));
			}
		if (pnExtraSessionActual)
			{
			*pnExtraSessionActual = 0;
			}
		return false;
		}

	// if you have persistent storage, you shold provide a function that
	// saves session info to persistent storage. This will be called
	// after a successful join or a MAC message update.
	virtual void NetSaveSessionInfo(
			const SessionInfo &SessionInfo,
			const uint8_t *pExtraSessionInfo,
			size_t nExtraSessionInfo
			)
		{
		// default: do nothing.
		}

	// Save FCntUp value (the uplink frame counter) (spelling matches
	// LoRaWAN spec).
	// If you have persistent storage, you should provide this function
	virtual void NetSaveFCntUp(
			uint32_t uFcntUp
			)
		{
		// default: no nothing.
		}

	// save FCntDown value (the downlink frame counter) (spelling matches
	// LoRaWAN spec).  If you have persistent storage, you should provide
	// this function.
	virtual void NetSaveFCntDown(
			uint32_t uFcntDown
			)
		{
		// default: do nothing.
		}

	// return true if verbose logging is enabled.
	bool LogVerbose()
		{
		return (this->m_ulDebugMask & LOG_VERBOSE) != 0;
		}
	
	uint32_t m_ulDebugMask;
        bool m_fTxPending;

        SendBufferCbFn *m_pSendBufferDoneFn;
        void *m_pSendBufferDoneCtx;

        void completeTx(bool fStatus)
                {
                SendBufferCbFn * const pSendBufferDoneFn = this->m_pSendBufferDoneFn;

                this->m_pSendBufferDoneFn = nullptr;
                if (pSendBufferDoneFn != nullptr)
                        (*pSendBufferDoneFn)(this->m_pSendBufferDoneCtx, true);
                };

	ReceiveBufferCbFn *m_pReceiveBufferFn;
	void *m_pReceiveBufferCtx;

private:

        // this is a 'global' -- it gives us a way to bootstrap
        // back into C++ from the LMIC code.
	static Arduino_LoRaWAN *pLoRaWAN;

        void StandardEventProcessor(
            uint32_t ev
            );

	struct Listener
		{
		ARDUINO_LORAWAN_EVENT_FN *pEventFn;
		void *pContext;

		void ReportEvent(uint32_t ev)
			{
			this->pEventFn(this->pContext, ev);
			}
		};

	Listener m_RegisteredListeners[4];
	uint32_t m_nRegisteredListeners;

	// since the LMIC code is not really obvious as to which events
        // update the downlink count, we simply watch for changes.
	uint32_t m_savedFCntDown;
        };

/****************************************************************************\
|
|	Eventually this will get removed for "free" builds. But if you build
|	in the Arduino enfironment, this is going to get hard to override.
|
\****************************************************************************/

#if MCCIADK_DEBUG || 1
# define ARDUINO_LORAWAN_PRINTF(a_check, a_fmt, ...)    \
    do  {                                               \
        if (this->a_check())                            \
                {                                       \
                this->LogPrintf(a_fmt, ## __VA_ARGS__); \
                }                                       \
        } while (0)
#else
# define ARDUINO_LORAWAN_PRINTF(a_check, a_fmt, ...)	\
    do { ; } while (0)
#endif

/**** end of Arduino_LoRaWAN.h ****/
#endif /* _ARDUINO_LORAWAN_H_ */
