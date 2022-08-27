//
//  ViewController+AudioController.h
//  tone_barrier
//
//  An Objective-C class extension that declares properties related to
//  audio configuration and playback
//  used by the ViewController class
//
//  Created by Xcode Developer on 8/3/22.
//

#import "ViewController.h"


NS_ASSUME_NONNULL_BEGIN

static AVAudioSourceNodeRenderBlock (^audio_renderer)(void) = ^ AVAudioSourceNodeRenderBlock {
    __block Float32 theta = 0.f;
    const Float32 frequency = 440.f;
    const Float32 sampleRate = 48000.f;
    const Float32 amplitude = 0.25f;
    const Float32 M_PI_SQR = 2.f * M_PI;
    
    return ^OSStatus(BOOL * _Nonnull isSilence, const AudioTimeStamp * _Nonnull timestamp, AVAudioFrameCount frameCount, AudioBufferList * _Nonnull outputData) {
                Float32 theta_increment = M_PI_SQR * frequency / sampleRate;
                Float32 * buffer = (Float32 *)outputData->mBuffers[0].mData;
                for (AVAudioFrameCount frame = 0; frame < frameCount; frame++)
                {
                    buffer[frame] = sin(theta) * amplitude;
                    theta += theta_increment;
                    !(theta > M_PI_SQR) ?: (theta -= M_PI_SQR);
                }
                return (OSStatus)noErr;
            };
};

static AVAudioSourceNode * (^audio_source)(AVAudioSourceNodeRenderBlock) = ^ AVAudioSourceNode * (AVAudioSourceNodeRenderBlock audio_renderer) {
    AVAudioSourceNode * source_node = [[AVAudioSourceNode alloc] initWithRenderBlock:audio_renderer];

    return source_node;
};

typedef typeof(void(^(^)(NSNotification *, void(^)(NSNotification *)))(void)) notification_observer;
typedef typeof(void(^)(void)) notification;
void(^(^(^notification_observation)(NSNotificationCenter *, NSOperationQueue *))(NSNotification *, void(^)(NSNotification *)))(void)  = ^ (NSNotificationCenter * notification_center, NSOperationQueue * operation_queue) {
    return ^ (NSNotification * observed_notification, typeof(void(^)(NSNotification *))notification_handler) {
        return ^{
            [notification_center addObserverForName:observed_notification.name object:observed_notification.object queue:operation_queue usingBlock:notification_handler];
        };
    };
};
static void (^observe_notifications)(NSArray<notification> *) = ^ (NSArray<notification> * notifications) {
    for (notification observed_notification in notifications) observed_notification();
};

// To-Do: Create am NSNotificationObserver block type that observes notifications when passed an object to observe
//        It should return another block that, when executed, removes the notification observer

static AVAudioSession * (^audio_session)(void) = ^ AVAudioSession * {
    AVAudioSession * session = [AVAudioSession sharedInstance];
    
    @try {
        __autoreleasing NSError *error = nil;
        [session setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeDefault options:AVAudioSessionCategoryOptionDefaultToSpeaker error:&error];
        [session setSupportsMultichannelContent:TRUE  error:&error];
        [session setPreferredInputNumberOfChannels:2  error:&error];
        [session setPreferredOutputNumberOfChannels:2 error:&error];
        [session setPrefersNoInterruptionsFromSystemAlerts:TRUE error:&error]; // TO-DO: Make this a user-specified preference
        
        !(!error) ?: ^ (NSError ** error_t) {
            printf("Error configuring audio session:\n\t%s\n", [[*error_t debugDescription] UTF8String]);
            NSException* exception = [NSException
                                      exceptionWithName:(*error_t).domain
                                      reason:(*error_t).localizedDescription
                                      userInfo:@{@"Error Code" : @((*error_t).code)}];
            @throw exception;
        }(&error);
    } @catch (NSException *exception) {
        printf("Exception configuring audio session:\n\t%s\n\t%s\n\t%lu",
               [exception.name UTF8String],
               [exception.reason UTF8String],
               ((NSNumber *)[exception.userInfo valueForKey:@"Error Code"]).unsignedIntegerValue);
    } @finally {
        // Setup notifications
        
        notification_observer audio_session_notification_observer = notification_observation([NSNotificationCenter defaultCenter], [NSOperationQueue mainQueue]);
        
        notification observe_audio_session_interruption_notification = audio_session_notification_observer([NSNotification notificationWithName:(NSNotificationName)AVAudioSessionInterruptionNotification object:session], ^(NSNotification * notification) {
            UInt8 theInterruptionType = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
            NSLog(@"Session interrupted > --- %s ---\n", theInterruptionType == AVAudioSessionInterruptionTypeBegan ? "Begin Interruption" : "End Interruption");
            static BOOL _isSessionInterrupted;
            switch (theInterruptionType) {
                case AVAudioSessionInterruptionTypeBegan: {
                    //                _isSessionInterrupted = [_engine isRunning];
                    //                !(_isSessionInterrupted) ?: [self toggleAudioEngineRunningStatus:((ViewController *)[[[[UIApplication sharedApplication] delegate] window] rootViewController]).playPauseButton];
                    break;
                }
                    
                case AVAudioSessionInterruptionTypeEnded: {
                    //                !(_isSessionInterrupted) ?: [self toggleAudioEngineRunningStatus:((ViewController *)[[[[UIApplication sharedApplication] delegate] window] rootViewController]).playPauseButton];
                    //                _isSessionInterrupted = FALSE;
                    break;
                }
                    
                default:
                    break;
            }
        });
        
        notification observe_audio_route_change_notification = audio_session_notification_observer([NSNotification notificationWithName:(NSNotificationName)AVAudioSessionRouteChangeNotification object:session], ^(NSNotification * notification) {
            UInt8 reasonValue = [[notification.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] intValue];
            AVAudioSessionRouteDescription *routeDescription = [notification.userInfo valueForKey:AVAudioSessionRouteChangePreviousRouteKey];
            
            NSLog(@"Route change:");
            switch (reasonValue) {
                case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
                    NSLog(@"     NewDeviceAvailable");
                    break;
                case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
                    NSLog(@"     OldDeviceUnavailable");
                    break;
                case AVAudioSessionRouteChangeReasonCategoryChange:
                    NSLog(@"     CategoryChange");
                    NSLog(@"     New Category: %@", [[AVAudioSession sharedInstance] category]);
                    break;
                case AVAudioSessionRouteChangeReasonOverride:
                    NSLog(@"     Override");
                    break;
                case AVAudioSessionRouteChangeReasonWakeFromSleep:
                    NSLog(@"     WakeFromSleep");
                    break;
                case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
                    NSLog(@"     NoSuitableRouteForCategory");
                    break;
                default:
                    NSLog(@"     ReasonUnknown");
            }
            
            NSLog(@"Previous route:\n");
            NSLog(@"%@", routeDescription);
        });

        observe_notifications(@[observe_audio_session_interruption_notification, observe_audio_route_change_notification]);
        
    }
    
    return session;
};

static AVAudioEngine * (^audio_engine)(AVAudioSourceNode *) = ^(AVAudioSourceNode * audio_source) {
    AVAudioEngine * audio_engine = [[AVAudioEngine alloc] init];
    [audio_engine attachNode:audio_source];
    [audio_engine connect:audio_source to:audio_engine.mainMixerNode format:nil];
    [audio_engine prepare];
    
    return audio_engine;
};

typedef typeof(bool(^)(void)) audio_state;
typedef typeof(audio_state(^)(AVAudioEngine *, AVAudioSession *)) audio_state_ref;
audio_state_ref audio_state_ref_init = ^ (AVAudioEngine * audio_engine, AVAudioSession * audio_session) {
    __block NSError * error = nil;
    return ^ bool {
        return ([audio_session setActive:(((![audio_engine isRunning]) && ^ BOOL { [audio_engine startAndReturnError:&error]; return (!error) ? ([audio_engine isRunning]) : FALSE; }()) || ^ BOOL { [audio_engine stop]; return ([audio_engine isRunning]); }()) error:&error]) & [audio_engine isRunning];
    };
};

typedef typeof(void(^)(audio_state, ...)) audio_control;
audio_control play_pause_button_audio_control = ^ (audio_state audio_state_play_pause_button, ...) {
    
};

audio_control lock_screen_audio_control = ^ (audio_state lock_screen_play_pause_button, ...) {
    va_list argp;
    va_start(argp, lock_screen_play_pause_button);
    MPNowPlayingInfoCenter * nowPlayingInfoCenter = va_arg(argp, MPNowPlayingInfoCenter *);
    MPRemoteCommandCenter  * remoteCommandCenter  = va_arg(argp, MPRemoteCommandCenter  *);

    NSMutableDictionary<NSString *, id> * nowPlayingInfo = [[NSMutableDictionary alloc] initWithCapacity:2];
    [nowPlayingInfo setObject:@"ToneBarrier" forKey:MPMediaItemPropertyTitle];
    [nowPlayingInfo setObject:(NSString *)@"James Alan Bush" forKey:MPMediaItemPropertyArtist];
    [nowPlayingInfo setObject:(NSString *)@"The Life of a Demoniac" forKey:MPMediaItemPropertyAlbumTitle];
    
    static UIImage * image;
    MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithImage:(image = [UIImage imageNamed:@"WaveIcon"])];
    [nowPlayingInfo setObject:(MPMediaItemArtwork *)artwork forKey:MPMediaItemPropertyArtwork];

    [(nowPlayingInfoCenter = [MPNowPlayingInfoCenter defaultCenter]) setNowPlayingInfo:(NSDictionary<NSString *,id> * _Nullable)nowPlayingInfo];

    MPRemoteCommandHandlerStatus (^remote_command_handler)(MPRemoteCommandEvent * _Nonnull) = ^ MPRemoteCommandHandlerStatus (MPRemoteCommandEvent * _Nonnull event) {
        __block NSError * error = nil;
//        [nowPlayingInfoCenter setPlaybackState:/* toggle_play_pause_block */];
        return (!error) ? MPRemoteCommandHandlerStatusSuccess : MPRemoteCommandHandlerStatusCommandFailed;
    };

    [[(remoteCommandCenter = [MPRemoteCommandCenter sharedCommandCenter]) playCommand] addTargetWithHandler:remote_command_handler];
    [[remoteCommandCenter stopCommand] addTargetWithHandler:remote_command_handler];
    [[remoteCommandCenter pauseCommand] addTargetWithHandler:remote_command_handler];
    [[remoteCommandCenter togglePlayPauseCommand] addTargetWithHandler:remote_command_handler];

    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
};

static void (^(^(^update_audio_controls)(MPNowPlayingInfoCenter *, MPRemoteCommandCenter *))(UIButton *))(bool(^)(void)) = ^ (MPNowPlayingInfoCenter * now_playing_info_center, MPRemoteCommandCenter * remote_command_center) {
    return ^ (UIButton * playback_button) {
        return ^ (bool(^audio_state)(void)) {
        };
    };
};
//
//    return ^ (bool(^update_playback_interface)(void)) {
//        dispatch_async(dispatch_get_main_queue(), ^{
//            [playback_button setSelected:update_playback_interface()];
//        });
//    };
//};



typedef CFTypeRef _Nonnull (^stored_object)(void);
stored_object (^ _Nonnull store_object)(CFTypeRef) = ^ (CFTypeRef storable_object) {
    return ^ CFTypeRef (void) {
        return (__bridge CFTypeRef)CFBridgingRelease(storable_object);
    };
};

typedef typeof(CFTypeRef(^)(void)) update_control_block_ref;
typedef typeof(CFTypeRef(^ _Nonnull const (* _Nonnull restrict))(void)) update_control_block_ref_t;
typedef typeof(void(^ _Nonnull const)(bool)) update_control_block;
typedef typeof(void(^ _Nonnull const (* _Nonnull restrict))(bool)) update_control_block_t;

static void (^(^(^(^audio_controller)(audio_state))(unsigned long))(CFTypeRef(^)(void)))(void) = ^ (audio_state state) {
    return ^ (unsigned long object_count) {
        typedef CFTypeRef objects[object_count];
        typeof(objects) objects_ptr[object_count];
        return ^ (CFTypeRef * objects_t) {
            return ^ (CFTypeRef(^write_object)(void)) {
                for (unsigned int index = 0; index < object_count; index++) {
                    *((CFTypeRef *)objects_t + index) = write_object();
                }
                return ^{
                    for (unsigned int index = 0; index < object_count; index++) {
                        ((__bridge typeof(void(^)(bool)))(store_object(*((CFTypeRef *)objects_t + index))()))(state());
                    }
                };
            };
        }(objects_ptr);
    };
};

// ------------------------------------------------------------------

//id (^retainable_object)(id(^)(void)) = ^ id (id(^object)(void)) {
//    return ^{
//        return object();
//    };
//};
//
//id (^(^retain_object)(id(^)(void)))(void) = ^ (id(^retainable_object)(void)) {
//    id retained_object = retainable_object();
//    return ^ id {
//        return retained_object;
//    };
//};
//
//typedef typeof(unsigned long (^)(unsigned long)) recursive_iterator;
//static void (^(^_iterator_)(const unsigned long))(id(^)(void)) = ^ (const unsigned long object_count) {
//    typeof(id(^)(void)) retained_objects_ref;
//    return ^ (id * retained_objects_t) {
//        return ^ (id(^object)(void)) {
//            //            return ^ (void (^(^retain_objects)(unsigned long))(void(^)(void))) {
//            //                return ^ (unsigned long index) {
//            //                    return retain_objects(index)(^{
//            for (int index = 0 ; index < object_count; index++) {
//                printf("retained_object: %p\n", (*((id * const)retained_objects_t + index) = retain_object(retainable_object(object))));
//                ((typeof(update_control_block))(*((id * const)retained_objects_t + index)))(FALSE);
//            }
//            //        });
//            //                }(object_count);
//            //            };
//        };
//    }((id *)&retained_objects_ref);
//};

/*
 id (^player_node_object)(void) = ^ id {
 AVAudioPlayerNode * player = [[AVAudioPlayerNode alloc] init];
 [player setRenderingAlgorithm:AVAudio3DMixingRenderingAlgorithmAuto];
 [player setSourceMode:AVAudio3DMixingSourceModeAmbienceBed];
 [player setPosition:AVAudioMake3DPoint(0.0, 0.0, 0.0)];
 
 return player;
}; iterator(2)(player_node_object);
 
 */


// ------------------------------------------------------------------

//static void (^(^(^(^audio_controller)(audio_state))(unsigned int))(update_control_block_ref))(void) = ^ (audio_state state) {
//    return ^ (unsigned int control_count) {
//        typedef CFTypeRef update_control_blocks[control_count];
//        typeof(update_control_blocks) update_control_blocks_ptr[control_count];
//        return ^ (CFTypeRef * update_control_blocks_t) {
//            __block unsigned long (^recursive_block)(unsigned long);
//            return ^ (update_control_block_ref update_control_block) {
//                return ^ (unsigned long offset) {
////                    *((CFTypeRef *)update_control_blocks_t) = CFBridgingRetain((__bridge id _Nullable)(update_control_block()));
////                    //                    *(update_control_blocks_t + offset) = &update_control_block;
////                    (recursive_block = ^ unsigned long (unsigned long index) {
////                        //                            update_control_block_ref update_control = *(*(update_control_blocks_t + 0));
////                        update_control_block update_control = (*((CFTypeRef *)update_control_blocks_t + index));
////                        (update_control)(state()); // WRONG: capture the return value of state and then pass that value in
////                        return (unsigned long)(0UL ^ (unsigned long)(recursive_block)(~-index));
////                    })(~-offset);
//                };
//            }(control_count);
//        };
//    }(&update_control_blocks_ptr[0]);
//};
//static void (^ _Nonnull (^ _Nonnull const (* _Nonnull restrict audio_controller_t))(void))(void(^)(bool)) = &audio_controller;

/*
 
 */


static int c = 0;

static void (^(^(^(^(^array_pointer_test)(unsigned int))(CFTypeRef(^)(void)))(void(^)(CFTypeRef)))(CFTypeRef(^)(CFTypeRef)))(bool(^)(CFTypeRef)) = ^ (unsigned int object_count) {
    typedef CFTypeRef objects[object_count];
    typeof(objects) objects_ptr[object_count];
    return ^ (CFTypeRef * objects_t) {
        
        __block void (^recurse)(void(^__strong)(CFTypeRef)) = ^{
            __block unsigned long (^recursive_block)(unsigned long);
            return ^ (void(^block)(CFTypeRef)) {
                (recursive_block = ^ unsigned long (unsigned long index) {
                    printf("index == %lu\t", index);
                    block((objects_t + (index * sizeof(CFTypeRef))));
                    return (unsigned long)(index ^ 0UL) && (unsigned long)recursive_block(~-index);
                })(~-object_count);
            };
        }();
        
        return ^ (CFTypeRef(^write_object)(void)) {
            for (unsigned int index = 0; index < object_count; index++) {
                *((CFTypeRef *)objects_t + index) = CFBridgingRetain((__bridge id _Nullable)(write_object()));
                printf("Stored number_write = %lu\n", [(NSNumber *)((__bridge id)(*((CFTypeRef *)objects_t + index))) unsignedLongValue]);
            }
            return ^ (void(^read_object)(CFTypeRef)) {
                for (unsigned int index = 0; index < object_count; index++) {
                    read_object((*((CFTypeRef *)objects_t + index)));
                    printf("Stored number_read = %lu\n", [(NSNumber *)((__bridge id)(*((CFTypeRef *)objects_t + index))) unsignedLongValue]);
                }
                return ^ (CFTypeRef(^modify_object)(CFTypeRef)) {
                    for (unsigned int index = 0; index < object_count; index++) {
                        *((CFTypeRef *)objects_t + index) = CFBridgingRetain((__bridge id _Nullable)(modify_object((*((CFTypeRef *)objects_t + index)))));
                        printf("modified_number = %lu\n", [(NSNumber *)((__bridge id)(*((CFTypeRef *)objects_t + index))) unsignedLongValue]);
                    }
                    return ^ (bool(^filter_object)(CFTypeRef)) {
                        unsigned long unfiltered_object_count = object_count;
                        for (unsigned int index = 0; index < unfiltered_object_count; index++) {
                            CFTypeRef subject_object = (*((CFTypeRef *)objects_t + index));
                            (filter_object(subject_object)) ? ^{ printf("number %lu is odd\n", [(__bridge NSNumber *)subject_object unsignedLongValue]); (*((CFTypeRef *)objects_t + index) = CFBridgingRetain((__bridge id _Nullable)(subject_object))); }()
                            : ^ (unsigned int * object_count_t){ *object_count_t -= 1; }(&object_count);
                        }
                        array_pointer_test(object_count)(^ CFTypeRef {
                            __block NSNumber * number = [[NSNumber alloc] initWithUnsignedLong:c++];
                            printf("Sent number_write == %lu\n", [number unsignedLongValue]);
                            return (__bridge CFTypeRef)(number);
                        });
                        printf("modified object_count == %lu\n", object_count);
                    };
                };
            };
        };
    }(objects_ptr);
};

// Sample usage

//    array_pointer_test(10)(^ CFTypeRef {
//        __block NSNumber * number = [[NSNumber alloc] initWithUnsignedLong:c++];
//        printf("Sent number_write == %lu\n", [number unsignedLongValue]);
//        return (__bridge CFTypeRef)(number);
//    })(^ (CFTypeRef number) {
//        printf("Returned number_read == %lu\n", [(__bridge NSNumber *)number unsignedLongValue]);
//    })(^ CFTypeRef (CFTypeRef number) {
//        number = (__bridge CFTypeRef)([[NSNumber alloc] initWithUnsignedLong:c++]);
//        return (CFTypeRef)(number);
//    })(^ bool (CFTypeRef number) {
//        return ([(__bridge NSNumber *)number unsignedLongValue] % 2);
//    });

/*
 
 */

@interface ViewController ()



@end

NS_ASSUME_NONNULL_END
