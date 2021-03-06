/* Copyright 2014 michal.mielczynski@gmail.com. All rights reserved.
 *
 *
 * RayPump Client software might be distributed under GNU GENERAL PUBLIC LICENSE
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAL MIELCZYNSKI ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL MICHAL MIELCZYNSKI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Michal Mielczynski.
 */

#include "commoncode.h"

bool Globals::SERVER_VIRTUALIZED = false;
QString Globals::SERVER_HOST_NAME  = ""; // used only if Globals::SERVER_VIRTUALIZED is true
/// @todo this IP should be read from http://raypump.com/headnode.txt before we actually try to connect
QString Globals::SERVER_IP  = "185.7.171.203"; // used only if Globals::SERVER_VIRTUALIZED is false
QString Globals::BUFFER_DIRECTORY = QApplication::applicationDirPath() + "/BUFFER";
QString Globals::RENDERS_DIRECTORY = "RENDERS";
