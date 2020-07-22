#ifndef NMEA_SIMULATION_DATA_GENERATOR
#define NMEA_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

class NmeaAnalyzerSettings;

class NmeaSimulationDataGenerator
{
public:
	NmeaSimulationDataGenerator();
	~NmeaSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, NmeaAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channelss );

protected:
	NmeaAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	BitState mBitLow;
	BitState mBitHigh;
	U64 mValue;

	U64 mMpModeAddressMask;
	U64 mMpModeDataMask;
	U64 mNumBitsMask;

protected: //Nmea specific

	void CreateNmeaByte( U64 value );
	ClockGenerator mClockGenerator;
	SimulationChannelDescriptor mNmeaSimulationData;  //if we had more than one channel to simulate, they would need to be in an array
};
#endif // NMEA_SIMULATION_DATA_GENERATOR
