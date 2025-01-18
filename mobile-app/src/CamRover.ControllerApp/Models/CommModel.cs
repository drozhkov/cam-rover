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

using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace CamRover.ControllerApp.Models
{

	enum CommCommand
	{
		None,
		Stop = 's',
		Forward = '+',
		Reverse = '-',
		Left = 'l',
		Right = 'r',
		Flash = 'f',
		Ack = 'a',
		Set = 't',
		Deadzone = 'z'
	}


	public class CommModel
	{
		readonly record struct DiscoverResult( IPAddress Address, ushort ControlPortNo, ushort StreamPortNo );


		public event Func<Task>? DiscoveryStarted;
		public event Func<Task>? Discovered;
		public event Func<byte[], Task>? FrameReceived;
		public event Func<int, int, Task>? SpeedUpdated;
		public event Func<int, Task>? FpsUpdated;

		public uint MoveSpeedIncrement
		{
			get; set;
		} = 4;

		public uint CameraFlashDuty
		{
			get; set;
		}

		uint m_messageId;

		EventWaitHandle m_connectCommandEvent = new EventWaitHandle( false, EventResetMode.ManualReset );
		EventWaitHandle m_commEvent = new EventWaitHandle( false, EventResetMode.ManualReset );
		volatile CommCommand m_commCommand;
		volatile bool m_isStreamingActive;

		public int SpeedL
		{
			get; set;
		}

		public int SpeedR
		{
			get; set;
		}

		public uint Deadzone
		{
			get; set;
		} = 30;


		byte[] MessageMove( CommCommand cmd )
		{
			var id = Interlocked.Increment( ref m_messageId );
			var idBytes = BitConverter.GetBytes( id );
			return [6, idBytes[0], idBytes[1], idBytes[2], idBytes[3], (byte)cmd, (byte)this.MoveSpeedIncrement];
		}


		byte[] MessageCameraFlash()
		{
			var id = Interlocked.Increment( ref m_messageId );
			var idBytes = BitConverter.GetBytes( id );
			return [6, idBytes[0], idBytes[1], idBytes[2], idBytes[3], (byte)CommCommand.Flash, (byte)this.CameraFlashDuty];
		}


		byte[] MessageMoveStop()
		{
			var id = Interlocked.Increment( ref m_messageId );
			var idBytes = BitConverter.GetBytes( id );
			return [5, idBytes[0], idBytes[1], idBytes[2], idBytes[3], (byte)CommCommand.Stop];
		}


		byte[] MessageMoveSet()
		{
			var id = Interlocked.Increment( ref m_messageId );
			var idBytes = BitConverter.GetBytes( id );
			var speedLBytes = BitConverter.GetBytes( this.SpeedL );
			var speedRBytes = BitConverter.GetBytes( this.SpeedR );
			return [13
				, idBytes[0], idBytes[1], idBytes[2], idBytes[3]
				, (byte)CommCommand.Set
				, speedLBytes[0], speedLBytes[1], speedLBytes[2], speedLBytes[3]
				, speedRBytes[0], speedRBytes[1], speedRBytes[2], speedRBytes[3]];
		}


		byte[] MessageMoveDeadzone()
		{
			var id = Interlocked.Increment( ref m_messageId );
			var idBytes = BitConverter.GetBytes( id );
			var deadzone = BitConverter.GetBytes( this.Deadzone );
			return [9
				, idBytes[0], idBytes[1], idBytes[2], idBytes[3]
				, (byte)CommCommand.Deadzone
				, deadzone[0], deadzone[1], deadzone[2], deadzone[3]
				];
		}


		byte[] MessageAck()
		{
			var id = 0;
			var idBytes = BitConverter.GetBytes( id );
			return [5, idBytes[0], idBytes[1], idBytes[2], idBytes[3], (byte)CommCommand.Ack];
		}


		async Task OnDiscoveryStart()
		{
			var e = this.DiscoveryStarted;

			if (e != null)
			{
				await e();
			}
		}


		async Task OnDiscoveryComplete()
		{
			var e = this.Discovered;

			if (e != null)
			{
				await e();
			}
		}


		async Task OnFrameReceive( byte[] data )
		{
			var e = this.FrameReceived;

			if (e != null)
			{
				await e( data );
			}
		}


		async Task OnSpeedReceive( int l, int r )
		{
			var e = this.SpeedUpdated;

			if (e != null)
			{
				await e( l, r );
			}
		}


		async Task OnFpsUpdate( int fps )
		{
			var e = this.FpsUpdated;

			if (e != null)
			{
				await e( fps );
			}
		}


		async void StreamWorker( IPAddress address, ushort portNo )
		{
			IPEndPoint ip = new( address, portNo );

			while (m_isStreamingActive)
			{
				try
				{
					using var udpClient = new UdpClient( 0, AddressFamily.InterNetwork );
					//using var receiveCts = new CancellationTokenSource( TimeSpan.FromSeconds( 5 ) );
					int frameCount = 0;
					Stopwatch sw = Stopwatch.StartNew();

					while (m_isStreamingActive)
					{
						using var receiveCts = new CancellationTokenSource( TimeSpan.FromSeconds( 5 ) );
						//receiveCts.TryReset();
						var receiveTask = udpClient.ReceiveAsync( receiveCts.Token );
						udpClient.Send( MessageAck(), ip );
						var response = await receiveTask;
						await OnFrameReceive( response.Buffer );

						frameCount++;

						if (sw.Elapsed.TotalSeconds > 5)
						{
							await OnFpsUpdate( (int)(frameCount / sw.Elapsed.TotalSeconds) );
							frameCount = 0;
							sw.Restart();
						}
					}
				}
				catch (Exception x)
				{
				}
			}
		}


		async Task<DiscoverResult> Discover( string ip = "239.255.255.250", ushort portNo = 3703 )
		{
			var endPoint = new IPEndPoint( IPAddress.Parse( "239.255.255.250" ), portNo );

			while (true)
			{
				IPAddress address;
				ushort controlPortNo;
				ushort streamPortNo;

				try
				{
					using UdpClient udpClient = new UdpClient( 0, AddressFamily.InterNetwork );
					using var receiveCts = new CancellationTokenSource( TimeSpan.FromSeconds( 5 ) );
					var receiveTask = udpClient.ReceiveAsync( receiveCts.Token );

					await udpClient.SendAsync( Encoding.ASCII.GetBytes( "CAM-ROVER:PROBE" ), endPoint );

					var receiveResult = await receiveTask;
					var response = Encoding.ASCII.GetString( receiveResult.Buffer );

					if (response.StartsWith( "CAM-ROVER:PROBE_MATCH:" ))
					{
						address = receiveResult.RemoteEndPoint.Address;
						var tokens = response.Split( ':' );
						controlPortNo = ushort.Parse( tokens[2] );
						streamPortNo = ushort.Parse( tokens[3] );

						return new DiscoverResult( address, controlPortNo, streamPortNo );
					}
				}
				catch
				{
				}

				await Task.Delay( TimeSpan.FromSeconds( 5 ) );
			}
		}


		async void Receiver( UdpClient client, CancellationToken cancellationToken )
		{
			while (!cancellationToken.IsCancellationRequested)
			{
				try
				{
					var receiveResult = await client.ReceiveAsync( cancellationToken );

					//				 1
					// 0 1234 5 6789 0123
					if (receiveResult.Buffer.Length >= (receiveResult.Buffer[0] + 1))
					{
						int l = (int)((uint)receiveResult.Buffer[6] | ((uint)receiveResult.Buffer[7] << 8) | ((uint)receiveResult.Buffer[8] << 16) | ((uint)receiveResult.Buffer[9] << 24));
						int r = (int)((uint)receiveResult.Buffer[10] | ((uint)receiveResult.Buffer[11] << 8) | ((uint)receiveResult.Buffer[12] << 16) | ((uint)receiveResult.Buffer[13] << 24));
						await OnSpeedReceive( l, r );
					}
				}
				catch
				{
					await Task.Delay( TimeSpan.FromSeconds( 5 ) );
				}
			}
		}


		async void Worker()
		{
			while (true)
			{
				var receiverCts = new CancellationTokenSource();

				try
				{
					await OnDiscoveryStart();
					var discoverResult = await Discover();
					await OnDiscoveryComplete();

					m_connectCommandEvent.WaitOne();
					m_connectCommandEvent.Reset();

					m_isStreamingActive = true;
					ThreadPool.QueueUserWorkItem( ( _ ) => StreamWorker( discoverResult.Address, discoverResult.StreamPortNo ) );

					using var controlUdpClient = new UdpClient( 0, AddressFamily.InterNetwork );
					var ip = new IPEndPoint( discoverResult.Address, discoverResult.ControlPortNo );

					ThreadPool.QueueUserWorkItem( ( _ ) => Receiver( controlUdpClient, receiverCts.Token ) );

					while (true)
					{
						m_commEvent.WaitOne();
						m_commEvent.Reset();

						do
						{
							var command = m_commCommand;

							switch (command)
							{
								case CommCommand.Forward:
									{
										await controlUdpClient.SendAsync( MessageMove( CommCommand.Forward ), ip );
										//await controlUdpClient.SendAsync( Encoding.ASCII.GetBytes( "+" ), ip );
									}
									break;

								case CommCommand.Reverse:
									{
										await controlUdpClient.SendAsync( MessageMove( CommCommand.Reverse ), ip );
										//await controlUdpClient.SendAsync( Encoding.ASCII.GetBytes( "-" ), ip );
									}
									break;

								case CommCommand.Left:
									{
										await controlUdpClient.SendAsync( MessageMove( CommCommand.Left ), ip );
										//await controlUdpClient.SendAsync( Encoding.ASCII.GetBytes( "l" ), ip );
									}
									break;

								case CommCommand.Right:
									{
										await controlUdpClient.SendAsync( MessageMove( CommCommand.Right ), ip );
										//await controlUdpClient.SendAsync( Encoding.ASCII.GetBytes( "r" ), ip );
									}
									break;

								case CommCommand.Stop:
									{
										await controlUdpClient.SendAsync( MessageMoveStop(), ip );
									}
									break;

								case CommCommand.Set:
									{
										await controlUdpClient.SendAsync( MessageMoveSet(), ip );
									}
									break;

								case CommCommand.Deadzone:
									{
										await controlUdpClient.SendAsync( MessageMoveDeadzone(), ip );
									}
									break;

								case CommCommand.Flash:
									{
										await controlUdpClient.SendAsync( MessageCameraFlash(), ip );
									}
									break;
							}

							await Task.Delay( TimeSpan.FromMilliseconds( 200 ) );
						}
						while (new List<CommCommand>( [CommCommand.Forward, CommCommand.Reverse, CommCommand.Left, CommCommand.Right] ).Contains( m_commCommand ));

						//{
						//	await controlUdpClient.SendAsync( MessageMoveStop(), ip );
						//}
					}
				}
				catch (Exception x)
				{
					await Task.Delay( TimeSpan.FromSeconds( 1 ) );
				}

				receiverCts.Cancel();
			}
		}


		public void Init()
		{
			ThreadPool.QueueUserWorkItem( ( _ ) => Worker() );
		}


		public void StartStream()
		{
			m_connectCommandEvent.Set();
		}


		internal void SendCommand( CommCommand command )
		{
			m_commCommand = command;
			m_commEvent.Set();
		}
	}

}
