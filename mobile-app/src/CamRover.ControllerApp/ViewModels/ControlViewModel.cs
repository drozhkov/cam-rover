/*
 *	Copyright (c) 2025 Denis Rozhkov <denis@rozhkoff.com>
 *	This file is part of cam-rover.
 *
 *	cam-rover is free software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	cam-rover is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *	Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along with
 *	cam-rover. If not, see <https://www.gnu.org/licenses/>.
 */

// SPDX-FileCopyrightText: 2025 Denis Rozhkov <denis@rozhkoff.com>
// SPDX-License-Identifier: GPL-3.0-or-later

using CamRover.ControllerApp.Models;
using CamRover.ControllerApp.Types;

using CommunityToolkit.Mvvm.Input;

namespace CamRover.ControllerApp.ViewModels
{

	public class ControlViewModel : ControllerAppViewModel
	{
		CommModel m_comm;

		public RelayCommand StartStreamingCommand
		{
			get;
		}

		public RelayCommand? StopStreamingCommand
		{
			get;
		}

		public AsyncRelayCommand OpenSettingsCommand
		{
			get;
		}

		public RelayCommand ForwardPressedCommand
		{
			get;
		}

		public RelayCommand ForwardReleasedCommand
		{
			get;
		}

		public RelayCommand ReversePressedCommand
		{
			get;
		}

		public RelayCommand ReverseReleasedCommand
		{
			get;
		}

		public RelayCommand LeftPressedCommand
		{
			get;
		}

		public RelayCommand LeftReleasedCommand
		{
			get;
		}

		public RelayCommand RightPressedCommand
		{
			get;
		}

		public RelayCommand RightReleasedCommand
		{
			get;
		}

		public RelayCommand StopCommand
		{
			get;
		}

		public RelayCommand CenterCommand
		{
			get;
		}

		public RelayCommand RotateFrameCommand
		{
			get;
		}

		volatile bool m_isConnected;
		public bool IsConnected
		{
			get
			{
				return m_isConnected;
			}
			set
			{
				m_isConnected = value;
				RaisePropertyChanged();
			}
		}

		volatile bool m_canConnect;
		public bool CanConnect
		{
			get
			{
				return m_canConnect;
			}
			set
			{
				m_canConnect = value;
				RaisePropertyChanged();
			}
		}

		Func<Action, Task> m_dispatcher = ( a ) => Task.Run( a );
		Action m_refreshFrame = () => { };

		public FrameDrawable FrameDrawable
		{
			get;
		} = new FrameDrawable();

		public uint FlashDuty
		{
			get
			{
				return m_comm.CameraFlashDuty;
			}
			set
			{
				m_comm.CameraFlashDuty = value;
				m_comm.SendCommand( CommCommand.Flash );
			}
		}

		int m_speedL;
		public int SpeedL
		{
			get
			{
				return m_speedL;
			}
			set
			{
				m_speedL = value;
				RaisePropertyChanged();
			}
		}

		int m_speedR;
		public int SpeedR
		{
			get
			{
				return m_speedR;
			}
			set
			{
				m_speedR = value;
				RaisePropertyChanged();
			}
		}

		int m_fps;
		public int Fps
		{
			get
			{
				return m_fps;
			}
			set
			{
				m_fps = value;
				RaisePropertyChanged();
			}
		}


		public ControlViewModel( CommModel comm )
		{
			m_comm = comm;

			m_comm.DiscoveryStarted += comm_DiscoveryStarted;
			m_comm.Discovered += comm_Discovered;
			m_comm.FrameReceived += comm_FrameReceived;
			m_comm.SpeedUpdated += comm_SpeedUpdated;
			m_comm.FpsUpdated += comm_FpsUpdated;

			this.StartStreamingCommand = new RelayCommand(
				() =>
				{
					m_comm.StartStream();
					this.IsConnected = true;
				},
				() => this.CanConnect );

			this.OpenSettingsCommand = new AsyncRelayCommand(
				async () =>
				{
					await Shell.Current.Navigation.PushAsync( IPlatformApplication.Current?.Services.GetService<Pages.SettingsPage>() );
				} );

			this.ForwardPressedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.Forward );
				} );

			this.ForwardReleasedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.None );
				} );

			this.ReversePressedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.Reverse );
				} );

			this.ReverseReleasedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.None );
				} );

			this.LeftPressedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.Left );
				} );

			this.LeftReleasedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.None );
				} );

			this.RightPressedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.Right );
				} );

			this.RightReleasedCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.None );
				} );

			this.StopCommand = new RelayCommand(
				() =>
				{
					m_comm.SendCommand( CommCommand.Stop );
				} );

			this.CenterCommand = new RelayCommand(
				() =>
				{
					var s = this.SpeedL + this.SpeedR;
					m_comm.SpeedL = s / 2;
					m_comm.SpeedR = s / 2;
					m_comm.SendCommand( CommCommand.Set );
				} );

			this.RotateFrameCommand = new RelayCommand(
				() =>
				{
					var d = this.FrameDrawable.RotateDegress;
					this.FrameDrawable.RotateDegress = (d > 180 ? -180 : d) + 90;
				} );
		}


		private async Task comm_FpsUpdated( int fps )
		{
			this.Fps = fps;
		}


		private async Task comm_SpeedUpdated( int l, int r )
		{
			this.SpeedL = l;
			this.SpeedR = r;
		}


		private async Task comm_FrameReceived( byte[] data )
		{
			this.FrameDrawable.SetFrame( data );
			await m_dispatcher( m_refreshFrame );
		}


		private async Task comm_Discovered()
		{
			await m_dispatcher(
				() =>
				{
					this.CanConnect = true;
					this.IsConnected = false;
					this.StartStreamingCommand.NotifyCanExecuteChanged();
				}
			);
		}


		private async Task comm_DiscoveryStarted()
		{
			await m_dispatcher(
				() =>
				{
					this.CanConnect = false;
					this.IsConnected = false;
					this.StartStreamingCommand.NotifyCanExecuteChanged();
				} );
		}


		public void Init( Func<Action, Task> dispatcher, Action refreshFrame )
		{
			m_dispatcher = dispatcher;
			m_refreshFrame = refreshFrame;
			m_comm.Init();
		}
	}

}
