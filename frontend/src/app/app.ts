import { Component, signal } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { TspMapComponent } from './components/tsp-map/tsp-map';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-root',
  imports: [CommonModule, TspMapComponent], 
  templateUrl: './app.html',
  styleUrl: './app.scss'
})
export class App {
  protected readonly title = signal('frontend');
}
