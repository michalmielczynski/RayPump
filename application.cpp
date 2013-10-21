/* Copyright 2013 Michal Mielczynski. All rights reserved.
 *
 * DISTRIBUTION OF THIS SOFTWARE, IN ANY FORM, WITHOUT WRITTEN PERMISSION FROM
 * MICHAL MIELCZYNSKI, IS ILLEGAL AND PROHIBITED BY LAW.
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

#include "application.h"

Application::Application(int &argc, char **argv):QApplication(argc, argv)
{
    _singular = new QSharedMemory("RayPumpUniqueName", this);
}

Application::~Application()
{
    if(_singular->isAttached())
        _singular->detach();
}

/// @todo  this method has been reported as buggy. We should reimplement more reliable way to manange singleton process
bool Application::lock()
{
    if(_singular->attach(QSharedMemory::ReadOnly)){
        _singular->detach();
        return false;
    }

    if(_singular->create(1))
        return true;

    return false;
}
