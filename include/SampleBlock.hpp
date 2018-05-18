/*
  Header for sample block;
*/
#include "Debug.hpp"
#include <vector>
#include <algorithm>

#ifndef TILER_SAMPLEBLOCK_H_
#define TILER_SAMPLEBLOCK_H_

/**
 * Performs some useful reductions/calculations on the sample block
 */
template<typename T> struct ReductionOperator 
{
    //ReductionOperator<T>();
    int numUsed = 0;
    T runningTotal = 0;
    T mMax = 0;
    T average_magnitude = 0;
    void operator()(const T & applied){
        numUsed++;
        runningTotal += applied;
        if(applied > mMax){
            mMax = applied;
        }
    }
    T RetAv(){
        if(numUsed == 0){
            return 0;
        } else{
            return runningTotal;
        }
    }

};

/**
 * Wrapper for Audio Samples as a block.
 * Will be the main unit for manipulating with
 * filters and transforms.
 */ 
template<typename T> class SampleBlock 
{
public:
  SampleBlock (std::vector<T> init);
 // ~SampleBlock();

  int GetSize();
  // Returns the number of samples for each channel.
  // Need copy constructor and copy assignment
  int ShiftBlock(int shiftHz);
  void PrintInfo();
private:
  T mMax;
  T mAverage;
  int mWidth;
  std::vector<T> mSamples;
  unsigned int mPitch;

};
   
/**
 * This will eventually be used to carry individual words, but will currently be represented
 * as a set block of samples that can perform math on eachother to check for comparison.
 */ 
template <class T> int SampleBlock<T>::GetSize()
    {
    return mSamples.size;
};

template<class T> SampleBlock<T>::SampleBlock (std::vector<T> init): mPitch(0)
    {
    mWidth = sizeof(T);
    mSamples = init;
    ReductionOperator<T> max = std::for_each(mSamples.begin(), mSamples.end(), ReductionOperator<T>());
    mMax = max.mMax;
    mAverage = max.RetAv();
    
};

/**
 * Perform pitch shift on block
 */
template<class T> int SampleBlock<T>::ShiftBlock(int shiftHz){
    return 0;
};

/**
 * Print the good stuff
 */
template<class T> void SampleBlock<T>::PrintInfo(){
    LOG("MAX ", mMax);
    LOG("AVERAGE ", mAverage);
    LOG("WIDTH ", mWidth);
    LOG("SIZE ", mSamples.size());
};
       

#endif // TILER_SAMPLEBLOCK_H_
