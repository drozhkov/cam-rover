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

using CamRover.ControllerApp.Types;
using CamRover.ControllerApp.ViewModels;

namespace CamRover.ControllerApp
{

	[XamlCompilation( XamlCompilationOptions.Compile )]
	public partial class ControlPage : ContentPage
	{
		ControlViewModel m_viewModel;


		public ControlPage( ControlViewModel viewModel )
		{
			m_viewModel = viewModel;

			InitializeComponent();
			this.BindingContext = m_viewModel;

			m_viewModel.Init( this.DispatchAsync,
				_frameView.Invalidate );
		}


		private void Forward_CurrentTouchStatusChanged( object sender, CommunityToolkit.Maui.Core.TouchStatusChangedEventArgs e )
		{
			if (e.Status == CommunityToolkit.Maui.Core.TouchStatus.Started)
			{
				m_viewModel.ForwardPressedCommand.Execute( null );
			}
			else
			{
				m_viewModel.ForwardReleasedCommand.Execute( null );
			}
		}


		private void Reverse_CurrentTouchStatusChanged( object sender, CommunityToolkit.Maui.Core.TouchStatusChangedEventArgs e )
		{
			if (e.Status == CommunityToolkit.Maui.Core.TouchStatus.Started)
			{
				m_viewModel.ReversePressedCommand.Execute( null );
			}
			else
			{
				m_viewModel.ReverseReleasedCommand.Execute( null );
			}
		}


		private void Left_CurrentTouchStatusChanged( object sender, CommunityToolkit.Maui.Core.TouchStatusChangedEventArgs e )
		{
			if (e.Status == CommunityToolkit.Maui.Core.TouchStatus.Started)
			{
				m_viewModel.LeftPressedCommand.Execute( null );
			}
			else
			{
				m_viewModel.LeftReleasedCommand.Execute( null );
			}
		}


		private void Right_CurrentTouchStatusChanged( object sender, CommunityToolkit.Maui.Core.TouchStatusChangedEventArgs e )
		{
			if (e.Status == CommunityToolkit.Maui.Core.TouchStatus.Started)
			{
				m_viewModel.RightPressedCommand.Execute( null );
			}
			else
			{
				m_viewModel.RightReleasedCommand.Execute( null );
			}
		}


		private void Stop_CurrentTouchStatusChanged( object sender, CommunityToolkit.Maui.Core.TouchStatusChangedEventArgs e )
		{
			if (e.Status == CommunityToolkit.Maui.Core.TouchStatus.Started)
			{
				m_viewModel.StopCommand.Execute( null );
			}
		}


		private void Center_CurrentTouchStatusChanged( object sender, CommunityToolkit.Maui.Core.TouchStatusChangedEventArgs e )
		{
			if (e.Status == CommunityToolkit.Maui.Core.TouchStatus.Started)
			{
				m_viewModel.CenterCommand.Execute( null );
			}
		}
	}

}
