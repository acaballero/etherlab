//
// Created by Angel Dust on 25/04/2021.
//

/*

 ADC Acquisition
 REAL TIME NORMALIZING AND FILTERING version with double buffering DMA. DOESN'T WORK because the FIR filter takes more time than the ADC/DMA cycle
FFT_STATUS adquireFFTAsync(ADC_HandleTypeDef *hadc1) {


    if (fft_status == FFT_STATUS_IDLE) {

        fft_status = FFT_STATUS_ADQUIRING;

        memset(fir_decimate_instance_I.pState,0,sizeof(firStateBufferI));
        memset(fir_decimate_instance_Q.pState,0,sizeof(firStateBufferQ));

        */
/* The FIR filter has a delay of (FFT_LPF_FIR_FILTER_NTAPS-1)/2 samples, so we
         * discard the first ((FFT_LPF_FIR_FILTER_NTAPS-1)/2)/DSP_BLOCK blocks *//*

        uint8_t discard_n_blocks=(uint8_t)(((FFT_LPF_FIR_FILTER_NTAPS-1)/2)/DSP_BLOCK)+1;

        uint16_t decimated_block_size = DSP_BLOCK / fft_decimation_factor;

        fft_type signalI[DSP_BLOCK],signalI2[decimated_block_size];
        fft_type signalQ[DSP_BLOCK],signalQ2[decimated_block_size];

        fft_buff_acq_ix=0;

        //logEvent(100,0);
        fft_dma_half_completed = false;
        HAL_ADC_Start_DMA(hadc1, (uint32_t *) &adc_block, DSP_BLOCK * 4);

        uint16_t start_adc_block_ix = 0;

        while (fft_buff_acq_ix < FFT_N) {

            logEvent(110,0);
            // Wait for half of the block to be acquired
            while (!fft_dma_half_completed);

            logEvent(120,0);
            fft_dma_half_completed = false;

            */
/* Normalize the ADC readings *//*

            for (uint16_t i = start_adc_block_ix; i < start_adc_block_ix+DSP_BLOCK; i++) {

                // Convert to signed int (substracting 2^10, the center value at the ADC's 12 bit resolution)
                adc_block[i].r = adc_block[i].r - (2 << 10);// + config.fft.DCOffset_I;
                adc_block[i].i = adc_block[i].i - (2 << 10);// + config.fft.DCOffset_Q;

                // Uncomment to create a pure sinusoid for testing
                //  double rads = 2.0 * PI * (((int64_t)config.f_carrier - (int64_t)5005000) / (double) (config.fft.sampling_khz*1000)) * (double) i;

                //   adc_block[i].r = ((int16_t) (0 + (config.fft.maxAmpl>>4) * cos(rads)));
                //   adc_block[i].i = ((int16_t) (0 + (config.fft.maxAmpl>>4) * sin(rads)));

                if (config.fft.view_mode == FFT_VIEW_SPECTRUM) {

                    // Scale by 2^3 to increase resolution when treating the buffer as fixed point int the fft calculation
                    adc_block[i].r = adc_block[i].r << FFT_SCALE_FACTOR;
                    adc_block[i].i = adc_block[i].i << FFT_SCALE_FACTOR;

                }

#if FFT_TYPE == FFT_TYPE_FLOAT
                adc_block_f[i-start_adc_block_ix].r = (float32_t) adc_block[i].r;
                adc_block_f[i-start_adc_block_ix].i = (float32_t) adc_block[i].i;
#endif

            }

            if (fft_decimation_factor > 1) {

#if FFT_TYPE == FFT_TYPE_FLOAT

                logEvent(121,fft_buff_acq_ix);
                // Separate the interleaved IQ signals into two arrays to FIR filter them
                unzipIQSamples(adc_block_f, signalI, signalQ, DSP_BLOCK);

                logEvent(122,fft_buff_acq_ix);
                arm_fir_decimate_f32(&fir_decimate_instance_Q, (float32_t *) signalQ, (float32_t *) signalQ2, DSP_BLOCK);
                arm_fir_decimate_f32(&fir_decimate_instance_I, (float32_t *) signalI, (float32_t *) signalI2, DSP_BLOCK);

                logEvent(123,fft_buff_acq_ix);
                // Write to the final adc_buffer in interleaved IQ format
                zipIQSamples(signalI2, signalQ2, adc_buff_f+fft_buff_acq_ix, decimated_block_size);
#else
                throw new Exception("Not implemented");
#endif

            }
            else {

                // Just copy the block (block_size*2 as these are interleaved IQ samples) to the final ADC buffer

#if FFT_TYPE == FFT_TYPE_FLOAT
                memcpy(adc_buff_f+fft_buff_acq_ix,adc_block_f,DSP_BLOCK*2*sizeof(float32_t));
#else
                throw new Exception("Not implemented");
#endif

            }

            logEvent(130,fft_buff_acq_ix);

            if (discard_n_blocks==0) {
                fft_buff_acq_ix += decimated_block_size;
            }
            else {
                discard_n_blocks--;
            }

            start_adc_block_ix = start_adc_block_ix ? 0 : DSP_BLOCK;

        }


       // logEvent(140,fft_buff_acq_ix);
        fft_status = FFT_STATUS_IDLE;
        HAL_ADC_Stop_DMA(hadc1);

    }

    return fft_status;
}


 ADC Acquisition
 REAL TIME NORMALIZING AND IIR filtering
 DOESN'T WORK because the IIR filter takes more time than the ADC/DMA cycle
FFT_STATUS adquireFFTAsync(ADC_HandleTypeDef *hadc1) {


    if (fft_status == FFT_STATUS_IDLE) {

        uint16_t decimated_block_size = DSP_BLOCK / fft_decimation_factor;


        fft_type signalI2[DSP_BLOCK];
        fft_type signalQ2[DSP_BLOCK];

        fft_status = FFT_STATUS_ADQUIRING;

        arm_biquad_cascade_df1_init_f32(&iir_instance_I, IIRFilterNumStages, IIRFilterCoefficients, IIRStateBufferI);
        arm_biquad_cascade_df1_init_f32(&iir_instance_Q, IIRFilterNumStages, IIRFilterCoefficients, IIRStateBufferQ);

        memset(IIRStateBufferI, 0, sizeof(IIRStateBufferI)); // Reset state to 0
        memset(IIRStateBufferQ, 0, sizeof(IIRStateBufferQ)); // Reset state to 0

        IIROutputI=0;
        IIROutputQ=0;

        */
/* The FIR filter has a delay of (FFT_LPF_FIR_FILTER_NTAPS-1)/2 samples, so we
         * discard the first ((FFT_LPF_FIR_FILTER_NTAPS-1)/2)/DSP_BLOCK blocks *//*

        uint8_t discard_n_blocks = (uint8_t) (((FFT_LPF_FIR_FILTER_NTAPS - 1) / 2) / DSP_BLOCK) + 1;


        fft_buff_acq_ix = 0;

        //logEvent(100,0);
        fft_dma_half_completed = false;
        HAL_ADC_Start_DMA(hadc1, (uint32_t *) &adc_block, DSP_BLOCK * 4);

        uint16_t start_adc_block_ix = 0;

        while (fft_buff_acq_ix < FFT_N) {

            logEvent(110, 0);
            // Wait for half of the block to be acquired
            while (!fft_dma_half_completed);

            logEvent(120, 0);
            fft_dma_half_completed = false;

            */
/* Normalize the ADC readings *//*

            for (uint16_t i = start_adc_block_ix; i < start_adc_block_ix + DSP_BLOCK; i++) {

                // Convert to signed int (substracting 2^10, the center value at the ADC's 12 bit resolution)
                adc_block[i].r = adc_block[i].r - (2 << 10);// + config.fft.DCOffset_I;
                adc_block[i].i = adc_block[i].i - (2 << 10);// + config.fft.DCOffset_Q;

                // Uncomment to create a pure sinusoid for testing
                //  double rads = 2.0 * PI * (((int64_t)config.f_carrier - (int64_t)5005000) / (double) (config.fft.sampling_khz*1000)) * (double) i;

                //   adc_block[i].r = ((int16_t) (0 + (config.fft.maxAmpl>>4) * cos(rads)));
                //   adc_block[i].i = ((int16_t) (0 + (config.fft.maxAmpl>>4) * sin(rads)));

                if (config.fft.view_mode == FFT_VIEW_SPECTRUM) {

                    // Scale by 2^3 to increase resolution when treating the buffer as fixed point int the fft calculation
                    adc_block[i].r = adc_block[i].r << FFT_SCALE_FACTOR;
                    adc_block[i].i = adc_block[i].i << FFT_SCALE_FACTOR;

                }

#if FFT_TYPE == FFT_TYPE_FLOAT
                adc_block_f[i - start_adc_block_ix].r = (float32_t) adc_block[i].r;
                adc_block_f[i - start_adc_block_ix].i = (float32_t) adc_block[i].i;
#endif

            }

            if (fft_decimation_factor > 1) {

#if FFT_TYPE == FFT_TYPE_FLOAT

                logEvent(121, fft_buff_acq_ix);
                // Separate the interleaved IQ signals into two arrays to FIR filter them
                unzipIQSamples(adc_block_f, signalI, signalQ, DSP_BLOCK);

                logEvent(122, fft_buff_acq_ix);

                // Filter
                arm_biquad_cascade_df1_f32(&iir_instance_I, signalI, signalI2, DSP_BLOCK);
                arm_biquad_cascade_df1_f32(&iir_instance_Q, signalQ, signalQ2, DSP_BLOCK);

                logEvent(123, fft_buff_acq_ix);

                // Decimate
                uint16_t dx=0;
                for (int d=0;d<DSP_BLOCK;d+=fft_decimation_factor) {
                    signalI2[dx]=signalI2[d];
                    signalQ2[dx]=signalQ2[d];
                    dx++;
                }


                logEvent(124, fft_buff_acq_ix);
                // Write to the final adc_buffer in interleaved IQ format
                zipIQSamples(signalI2, signalQ2, adc_buff_f + fft_buff_acq_ix, decimated_block_size);
#else
                throw new Exception("Not implemented");
#endif

            } else {

                // Just copy the block (block_size*2 as these are interleaved IQ samples) to the final ADC buffer

#if FFT_TYPE == FFT_TYPE_FLOAT
                memcpy(adc_buff_f + fft_buff_acq_ix, adc_block_f, DSP_BLOCK * 2 * sizeof(float32_t));
#else
                throw new Exception("Not implemented");
#endif

            }

            logEvent(130, fft_buff_acq_ix);

            if (discard_n_blocks == 0) {
                fft_buff_acq_ix += decimated_block_size;
            } else {
                discard_n_blocks--;
            }

            start_adc_block_ix = start_adc_block_ix ? 0 : DSP_BLOCK;

        }


        // logEvent(140,fft_buff_acq_ix);
        fft_status = FFT_STATUS_IDLE;
        HAL_ADC_Stop_DMA(hadc1);

        printLog();

    }

    return fft_status;
}
*/
