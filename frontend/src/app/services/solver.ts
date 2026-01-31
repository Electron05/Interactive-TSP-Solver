import { Injectable } from '@angular/core';
import { webSocket, WebSocketSubject } from 'rxjs/webSocket';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class SolverService {
  private socket$: WebSocketSubject<any>;

  constructor(){
    this.socket$ = webSocket({
      url: 'ws://localhost:8080'
    });
  }

  sendTSPData(distances: number[][]):void{
    this.socket$.next({type: 'solve', data:distances});
    
  }

  getSolverUpdates(): Observable<any> {
    return this.socket$.asObservable();
  }

  close(): void{
    this.socket$.complete();
  }

}
