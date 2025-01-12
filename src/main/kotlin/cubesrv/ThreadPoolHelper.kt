package cubesrv

import java.lang.Runtime
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.launch

val THREAD_COUNT : Int = Runtime.getRuntime().availableProcessors()

fun runInThreadPool(fn : suspend (threadNo : Int) -> Unit) = runBlocking {
    for(threadNo in 0 ..< THREAD_COUNT) {
        launch(Dispatchers.Default) {
            fn(threadNo)
        }
    }
}
