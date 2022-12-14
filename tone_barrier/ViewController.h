//
//  ViewController.h
//  tone_barrier
//
//  Created by Xcode Developer on 8/2/22.
//

@import UIKit;
@import Foundation;
@import AVFoundation;
@import AVFAudio;
@import AVRouting;
@import AVKit;
@import MediaPlayer;

NS_ASSUME_NONNULL_BEGIN

@interface ViewController : UIViewController

@property (weak, nonatomic) IBOutlet UIButton *playPauseButton;
@property (weak, nonatomic) IBOutlet AVRoutePickerView *routePickerView;

@end

NS_ASSUME_NONNULL_END
