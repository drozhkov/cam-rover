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

using Microsoft.Maui.Graphics.Platform;

namespace CamRover.ControllerApp.Types
{

	public class FrameDrawable : IDrawable
	{
		Microsoft.Maui.Graphics.IImage? m_image;


		public void Draw( ICanvas canvas, RectF dirtyRect )
		{
			if (m_image != null)
			{
				lock (this)
				{
					canvas.Rotate( -90, dirtyRect.Width / 2, dirtyRect.Height / 2 );
					var x = (dirtyRect.Width - m_image.Width) / 2;
					var y = (dirtyRect.Height - m_image.Height) / 2;
					canvas.DrawImage( m_image, x, y, m_image.Width, m_image.Height );
				}
			}
		}


		public void SetFrame( byte[] frame )
		{
			using (var stream = new MemoryStream( frame ))
			{
				lock (this)
				{
					m_image?.Dispose();
					m_image = PlatformImage.FromStream( stream, ImageFormat.Jpeg );
				}
			}
		}
	}

}
